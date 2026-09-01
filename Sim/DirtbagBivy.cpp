#include "DirtbagBivy.h"

#include <algorithm>

namespace dirtbag {
namespace {

int Slot(Spot s) { return static_cast<int>(s); }

// Five places, authored, each a different shape. Numbers from the 2D
// source's `BIVY_SPOTS`; the blurbs are its own words, because they are
// the content and rewriting them would be writing a different game.
const SpotDef kSpots[kSpotCount] = {
    {"The Lot",
     "The gravel by the trailhead sign. Free, central, and the city knows "
     "exactly where it is.",
     100.0, 0.0, true, 1.00, 0.00, 0.0, 0},
    {"The Upper Trailhead",
     "Fifteen minutes up the dirt road, past where the sedans turn around. "
     "Nobody comes up here at night.",
     100.0, 2.0, false, 1.35, 0.25, 0.0, 0},
    {"The Ridge Spot",
     "The pull-off the locals do not put in the guidebook. You had to be "
     "told about this one.",
     100.0, 6.0, false, 1.50, 0.50, 0.0, 0},
    {"The Truck Stop",
     "Lit like an operating theatre, loud all night, and not one person "
     "cares that you are living in a van.",
     92.0, -2.0, false, 0.70, 0.15, -8.0, 0},
    {"A Friend's Driveway",
     "Somebody said any time, and meant it. Power, a bathroom, and a door "
     "that locks.",
     100.0, 8.0, false, 0.35, 0.15, 0.0, 4},
};

}  // namespace

const SpotDef& Describe(Spot spot) {
  const int i = Slot(spot);
  return kSpots[i >= 0 && i < kSpotCount ? i : 0];
}

const char* SpotName(Spot spot) { return Describe(spot).name; }

bool SpotIsOpen(const Bivy& bivy, Spot spot, int today, bool aLocalSomewhere,
                double bestRapport, const BivyDials& dials) {
  const int i = Slot(spot);
  if (i < 0 || i >= kSpotCount) return false;
  // A cooldown is what stops a driveway becoming an address.
  const int cool = kSpots[i].cooldownDays;
  if (cool > 0 && bivy.lastSlept[i] > 0 && today - bivy.lastSlept[i] < cool) {
    return false;
  }
  if (spot == Spot::Ridge && !aLocalSomewhere) return false;
  if (spot == Spot::Driveway && bestRapport < dials.drivewayNeedsRapport) {
    return false;
  }
  return true;
}

std::string WhyNot(const Bivy& bivy, Spot spot, int today,
                   bool aLocalSomewhere, double bestRapport,
                   const BivyDials& dials) {
  if (SpotIsOpen(bivy, spot, today, aLocalSomewhere, bestRapport, dials)) {
    return "";
  }
  if (spot == Spot::Ridge && !aLocalSomewhere) {
    return "be a local somewhere first -- somebody has to tell you about it";
  }
  if (spot == Spot::Driveway && bestRapport < dials.drivewayNeedsRapport) {
    return "nobody knows you well enough to offer yet";
  }
  const int i = Slot(spot);
  const int left = kSpots[i].cooldownDays - (today - bivy.lastSlept[i]);
  return "not again this soon -- " + std::to_string(std::max(1, left)) +
         " more nights";
}

bool ParkAt(Bivy& bivy, Spot spot, int today, bool aLocalSomewhere,
            double bestRapport, const BivyDials& dials) {
  if (!SpotIsOpen(bivy, spot, today, aLocalSomewhere, bestRapport, dials)) {
    return false;
  }
  bivy.tonight = spot;
  return true;
}

BivyNight NightAt(Bivy& bivy, const Rng& worldRng, int day,
                  const BivyDials& dials) {
  const SpotDef& where = Describe(bivy.tonight);
  BivyNight out;
  out.rest = where.rest;
  out.psyche = where.psyche;
  // Priced at the pump rather than drawn from a tank, because that is the
  // fuel model this port has.
  out.fuel = FuelFor(where.driveHours);
  out.grime = where.grime;
  out.exposure = where.exposure;

  bivy.lastSlept[Slot(bivy.tonight)] = day;

  // **Consecutive nights, and only in the Lot.** Going anywhere else is
  // what resets it, which is the whole reason the other four exist.
  if (bivy.tonight == Spot::Lot) {
    bivy.lotNights++;
  } else {
    bivy.lotNights = 0;
  }

  if (where.tickets && bivy.lotNights > dials.ticketGrace) {
    const double odds = std::min(
        dials.ticketChanceMax,
        (bivy.lotNights - dials.ticketGrace) * dials.ticketChancePerNight);
    // Its own stream, so a parking ticket cannot shift the weather or
    // anybody's turnout.
    Rng rng = worldRng.Derive("bivy#" + std::to_string(day));
    if (rng.Chance(odds)) {
      bivy.ticketsOwed++;
      out.ticketed = true;
      if (!bivy.booted && bivy.ticketsOwed >= dials.bootAt) {
        bivy.booted = true;
        out.bootedTonight = true;
      }
    }
  }
  return out;
}

double WhatYouOwe(const Bivy& bivy, const BivyDials& dials) {
  double owed = bivy.ticketsOwed * dials.ticketFine;
  if (bivy.booted) owed += dials.impoundFee;
  return owed;
}

bool PayTheTickets(Bivy& bivy, double& cash, const BivyDials& dials) {
  const double owed = WhatYouOwe(bivy, dials);
  if (owed <= 0.0) return false;
  // **All of it or none of it.** The city does not do instalments, and a
  // boot you can half-pay is not a boot.
  if (cash < owed) return false;
  cash -= owed;
  bivy.ticketsOwed = 0;
  bivy.booted = false;
  return true;
}

std::string BivyWarning(const Bivy& bivy, const BivyDials& dials) {
  if (bivy.booted) {
    return "There is a boot on the van. Nothing moves until the city is paid.";
  }
  if (bivy.ticketsOwed >= dials.bootAt - 1) {
    return "One more ticket and they clamp it.";
  }
  if (bivy.ticketsOwed > 0) {
    return "$" + std::to_string(static_cast<int>(WhatYouOwe(bivy, dials))) +
           " in parking tickets.";
  }
  return "";
}

}  // namespace dirtbag

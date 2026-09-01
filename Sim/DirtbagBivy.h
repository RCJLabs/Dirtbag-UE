// Where you are parking tonight.
//
// **Ported from the 2D source** (`BIVY-1`), and it is the decision a person
// living like this makes every single day. The original's own line for it:
// *"Everybody who lives like this has a map of five or six of them."*
//
// ## Why this one was worth porting now
//
// It is a trade across seven axes at once, and **six of them are systems
// this port already has**:
//
//   rest       what you wake up with
//   psyche     what a place does to your head
//   tickets    whether the city can tag the van here
//   exposure   a multiplier on the cold-night sickness term
//   fuel       what it costs to get out there and back
//   grime      the truck stop has showers; the ridge does not
//   cooldown   a friend's driveway is not a place you live
//
// Every one of those was built for something else first. A spot table is
// the cheapest possible way to make somebody choose between them, which is
// why the 2D game gets so much out of it.
//
// ## The two halves
//
// **The spots** are a trade with no dominant option: the Lot is free,
// central and the city knows exactly where it is; the ridge is the best
// night's sleep in the game and costs six units of fuel and a road; a
// friend's driveway is better than either and you can only use it once
// every four nights, because a driveway is a favour and not an address.
//
// **The tickets** are what makes the free option cost something. Two nights
// grace, then a chance per night that climbs to a cap, a fine that accrues
// unpaid, and at three of them **the city clamps a boot on the van** and
// you are not driving anywhere until you clear them and the impound.
//
// Deliberately not here: **the knock** (`BIVY-2`, two in the morning and
// somebody is at the window), which is an event system with its own
// per-spot population and belongs in a pass of its own. The `knock`
// multiplier is not in this table because a dial nothing reads is a dial
// that lies -- see `notes/living-in-the-van.md` for the battery this
// project cut for the same reason an hour ago.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"
#include "DirtbagVan.h"

namespace dirtbag {

enum class Spot {
  Lot,          // the gravel by the trailhead sign
  UpperTrail,   // fifteen minutes up the dirt road
  Ridge,        // the pull-off the locals do not put in the guidebook
  TruckStop,    // lit like an operating theatre, and nobody cares
  Driveway,     // somebody said any time, and meant it
  kSpotCount
};
constexpr int kSpotCount = static_cast<int>(Spot::kSpotCount);

// What a place is, as data. Authored rather than generated -- five places
// are content, and the whole point is that each is a different shape.
struct SpotDef {
  const char* name;
  const char* blurb;
  double rest;      // energy you wake with; 100 is a full night
  double psyche;    // what it does to your head, in psyche points
  bool tickets;     // can the city tag the van here
  double exposure;  // multiplier on the cold-night sickness term
  // **Driving hours out and back, not fuel units.** The 2D game has a
  // tank and this port prices fuel per hour at the pump (`FuelFor`), so
  // porting the number rather than the meaning would have been a unit
  // error wearing a dial's clothes. The times come from the blurbs, which
  // state them: the upper trailhead really is *fifteen minutes up the dirt
  // road*.
  double driveHours;
  double grime;     // the truck stop has showers
  int cooldownDays; // 0 is every night
};

const SpotDef& Describe(Spot spot);
const char* SpotName(Spot spot);

struct BivyDials {
  // --- the city ---------------------------------------------------------
  // Free nights before anybody starts writing tickets. Two, because the
  // point is that the Lot works and then stops working.
  int ticketGrace = 2;

  // Added chance per consecutive night past the grace, and the cap. At 0.1
  // a week in the Lot is a coin flip every night, which is about right for
  // a place the city knows the address of.
  double ticketChancePerNight = 0.1;
  double ticketChanceMax = 0.4;

  double ticketFine = 25.0;

  // **Unpaid tickets before the city clamps a boot on the van.** Three, and
  // the third one is the difference between an annoyance and a career
  // problem: a booted van does not go to the crag.
  int bootAt = 3;
  double impoundFee = 60.0;

  // --- who lets you park in their driveway ------------------------------
  // How well somebody has to know you before it is an option. A driveway
  // is a favour, and the game should make you have earned it.
  double drivewayNeedsRapport = 0.6;
};

struct Bivy {
  Spot tonight = Spot::Lot;

  // The last day you slept at each, which is what a cooldown reads.
  int lastSlept[kSpotCount] = {0};

  // Consecutive nights in the Lot. What the ticket odds climb with, and
  // the only counter here that resets by going somewhere else.
  int lotNights = 0;

  int ticketsOwed = 0;
  bool booted = false;
};

// Is this one open to you tonight? The ridge wants you to be a local
// somewhere -- somebody has to tell you about it -- and the driveway wants
// somebody who knows you well enough to offer.
bool SpotIsOpen(const Bivy& bivy, Spot spot, int today, bool aLocalSomewhere,
                double bestRapport, const BivyDials& dials = BivyDials{});

// Why it is not, in one line, or empty when it is. The prompt needs to be
// able to say *nobody knows you well enough to offer yet* rather than just
// greying a row out.
std::string WhyNot(const Bivy& bivy, Spot spot, int today,
                   bool aLocalSomewhere, double bestRapport,
                   const BivyDials& dials = BivyDials{});

// Park here tonight. Refuses a spot that is not open to you.
bool ParkAt(Bivy& bivy, Spot spot, int today, bool aLocalSomewhere,
            double bestRapport, const BivyDials& dials = BivyDials{});

// What a night there did. Rolled at sleep, on its own stream.
struct BivyNight {
  double rest = 100.0;      // energy you wake with
  double psyche = 0.0;      // what it did to your head
  double fuel = 0.0;        // what getting there cost, at the pump
  double grime = 0.0;       // ...and what it did to how you smell
  double exposure = 1.0;    // multiplier on the cold-night term
  bool ticketed = false;
  bool bootedTonight = false;
};
BivyNight NightAt(Bivy& bivy, const Rng& worldRng, int day,
                  const BivyDials& dials = BivyDials{});

// Clear what you owe, and the impound if they have clamped you. Refuses
// unless you can cover all of it -- the city does not do instalments.
double WhatYouOwe(const Bivy& bivy, const BivyDials& dials = BivyDials{});
bool PayTheTickets(Bivy& bivy, double& cash,
                   const BivyDials& dials = BivyDials{});

// **Quiet until the city has noticed you**, like every other readout here.
std::string BivyWarning(const Bivy& bivy, const BivyDials& dials = BivyDials{});

}  // namespace dirtbag

#include "DirtbagVan.h"

#include <algorithm>

namespace dirtbag {

const char* VanPartName(VanPart part) {
  switch (part) {
    case VanPart::Tyres:    return "tyres";
    case VanPart::Brakes:   return "brakes";
    case VanPart::Belt:     return "belt";
    case VanPart::Battery:  return "battery";
    case VanPart::Radiator: return "radiator";
    case VanPart::Clutch:   return "clutch";
  }
  return "something";
}

int DriveVan(Van& van, const Rng& worldRng, int day, double hours,
             double airTempF, const VanDials& dials) {
  if (hours <= 0.0) return -1;
  van.hoursDriven += hours;

  for (int i = 0; i < kVanPartCount; i++) {
    double rate = hours / std::max(1.0, dials.lifeHours[i]);
    // The radiator is the one that cares what day it is. A warm afternoon
    // does more to it than a cool morning, which puts the gamble on exactly
    // the drives you were already unhappy about.
    if (static_cast<VanPart>(i) == VanPart::Radiator &&
        airTempF > dials.radiatorWarmF) {
      rate *= dials.radiatorHeatFactor;
    }
    van.parts[i].wear = std::min(1.0, van.parts[i].wear + rate);
  }

  // Its own stream, per day: the van getting on with rusting must never
  // shift the rng an attempt resolves on.
  Rng rng = worldRng.Derive("van#" + std::to_string(day));

  // Worst-first, so the thing you have been ignoring is the thing that goes.
  int order[kVanPartCount];
  for (int i = 0; i < kVanPartCount; i++) order[i] = i;
  std::sort(order, order + kVanPartCount, [&](int a, int b) {
    return van.parts[a].wear > van.parts[b].wear;
  });

  for (int k = 0; k < kVanPartCount; k++) {
    const int i = order[k];
    if (van.parts[i].failed) continue;
    if (van.parts[i].wear < dials.failsAbove) continue;

    // Chance climbs with how far past its life it is, so a part you keep
    // driving on is a part that will eventually pick its moment.
    const double past = (van.parts[i].wear - dials.failsAbove) /
                        std::max(0.001, 1.0 - dials.failsAbove);
    if (rng.Chance(dials.failChancePerDrive * past)) {
      van.parts[i].failed = true;
      return i;
    }
  }
  return -1;
}

bool VanRuns(const Van& van) {
  for (int i = 0; i < kVanPartCount; i++) {
    if (van.parts[i].failed) return false;
  }
  return true;
}

namespace {

// Every repair clears the failure and gives some life back. How much is the
// only difference between them.
void MendPart(VanPartState& p, double restores) {
  p.failed = false;
  p.wear = std::max(0.0, p.wear * (1.0 - restores));
}

}  // namespace

bool BodgeVan(Van& van, VanPart part, double& hoursSpent,
              const VanDials& dials) {
  // Always available. This is the promise that being broke cannot end a
  // save: a morning with a spanner beats a bill you cannot pay.
  VanPartState& p = van.parts[static_cast<int>(part)];
  MendPart(p, dials.bodgeRestores);
  hoursSpent += dials.bodgeHours;
  return true;
}

bool PatchVan(Van& van, VanPart part, double& cash, double& hoursSpent,
              const VanDials& dials) {
  const int i = static_cast<int>(part);
  VanPartState& p = van.parts[i];
  if (p.patches >= dials.patchesPerPart) return false;
  if (cash < dials.patchCost[i]) return false;

  cash -= dials.patchCost[i];
  p.patches++;
  MendPart(p, dials.patchRestores);
  hoursSpent += dials.patchHours;
  return true;
}

bool ReplaceVanPart(Van& van, VanPart part, double& cash, double& hoursSpent,
                    const VanDials& dials) {
  const int i = static_cast<int>(part);
  if (cash < dials.replaceCost[i]) return false;

  cash -= dials.replaceCost[i];
  VanPartState& p = van.parts[i];
  p.wear = 0.0;
  p.patches = 0;
  p.failed = false;
  hoursSpent += dials.replaceHours;
  return true;
}

int WorstVanPart(const Van& van, const VanDials& dials) {
  int worst = -1;
  double worstWear = dials.failsAbove * 0.6;   // below this, nothing to say
  for (int i = 0; i < kVanPartCount; i++) {
    if (van.parts[i].failed) return i;   // a failure outranks any warning
    if (van.parts[i].wear > worstWear) {
      worstWear = van.parts[i].wear;
      worst = i;
    }
  }
  return worst;
}

std::string VanText(const Van& van, const VanDials& dials) {
  const int i = WorstVanPart(van, dials);
  if (i < 0) return std::string();
  const std::string part = VanPartName(static_cast<VanPart>(i));

  if (van.parts[i].failed) {
    return "the " + part + " has gone. It is not going anywhere like this.";
  }
  if (van.parts[i].wear >= dials.failsAbove) {
    return "the " + part + " is on borrowed time";
  }
  return "the " + part + " is getting on";
}

}  // namespace dirtbag

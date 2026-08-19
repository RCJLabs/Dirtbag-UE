// The van: shelter, transport, and the reason seasons end early.
//
// The money loop measured flat before this (notes/phase3-economy.md): every
// cost was a small steady drip that one more shift absorbed, so working more
// always solved money and money never touched climbing. What was missing was
// a **lump** — a bill that arrives at the wrong moment, for an amount you
// did not have, on the morning conditions were finally good.
//
// That is what a van is. It wears by the hour driven, so the crag you drive
// to costs more than the one you walk to, and when something lets go it
// takes the day as well as the money.
//
// Three ways out of every failure, which is the important design decision:
//
//   bodge     free, costs hours, and does not hold for long
//   patch     cash, holds a while
//   replace   more cash, actually fixed
//
// The free option exists so that being broke is a bad situation rather than
// an unrecoverable one. A stranded player with no money can always spend a
// morning under the van with a spanner. That keeps the pressure honest:
// breakdowns take your season, never your save.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// Six things that go wrong, in the order a dirtbag learns to dread them.
enum class VanPart {
  Tyres,      // wear fastest, and you feel it before it fails
  Brakes,     // the one you cannot ignore
  Belt,       // pennies to replace, ruins the day when it goes
  Battery,    // dies without warning, cheapest fix of all
  Radiator,   // overheats, and warm days are when it happens
  Clutch,     // the season-ender
};
constexpr int kVanPartCount = 6;

const char* VanPartName(VanPart part);

struct VanDials {
  // Hours of driving a part lasts. Tyres go first, a clutch outlives
  // several sets of them, and none of it is on a schedule you chose.
  //
  // Sized against a real year: the season probe drives about 250 hours, and
  // the first pass at these numbers spent roughly thirteen part-lives in
  // that time — sixteen breakdowns a year, one every three weeks, which is
  // not a van, it is a running gag. The point of a breakdown is that it is
  // a lump: rare, expensive, and at the worst possible moment. Two or three
  // a year earns that; sixteen is weather.
  double lifeHours[kVanPartCount] = {240.0, 380.0, 165.0, 520.0, 320.0, 1100.0};

  // What it costs to bodge, patch and replace each. Bodging is always free
  // and always hours instead.
  double patchCost[kVanPartCount] = {40.0, 60.0, 15.0, 45.0, 55.0, 180.0};
  double replaceCost[kVanPartCount] = {220.0, 190.0, 35.0, 95.0, 160.0, 850.0};

  // Hours under the van, per tier. A bodge is most of a morning.
  double bodgeHours = 4.0;
  double patchHours = 1.5;
  double replaceHours = 2.5;

  // How much life each tier gives back. A bodge is a bodge.
  double bodgeRestores = 0.25;
  double patchRestores = 0.6;
  int patchesPerPart = 2;   // after that it wants replacing properly

  // Once a part is past this it can fail on any drive; the chance climbs
  // with how far past. Nothing fails out of the blue — the van tells you
  // first, which is what VanText is for.
  double failsAbove = 0.7;
  double failChancePerDrive = 0.16;

  // A radiator on a warm day. This is the one weather-coupled failure, and
  // it is deliberate: hot afternoons are already when you are not climbing,
  // and now they are when the drive home is a gamble.
  double radiatorWarmF = 82.0;
  double radiatorHeatFactor = 2.5;
};

struct VanPartState {
  double wear = 0.0;   // 0 new .. 1 done
  int patches = 0;
  bool failed = false; // stops the van until something is done about it
};

struct Van {
  VanPartState parts[kVanPartCount];
  double hoursDriven = 0.0;
};

// Drive somewhere. Wears everything a little, then rolls for failure on
// whatever is past its life. Returns the part that let go, or -1.
// `airTempF` is only read by the radiator.
int DriveVan(Van& van, const Rng& worldRng, int day, double hours,
             double airTempF, const VanDials& dials = VanDials{});

// Is the van going anywhere?
bool VanRuns(const Van& van);

// The three ways out. Each returns false if it cannot be done — bodging can
// always be done, which is the point of it.
bool BodgeVan(Van& van, VanPart part, double& hoursSpent,
              const VanDials& dials = VanDials{});
bool PatchVan(Van& van, VanPart part, double& cash, double& hoursSpent,
              const VanDials& dials = VanDials{});
bool ReplaceVanPart(Van& van, VanPart part, double& cash, double& hoursSpent,
                    const VanDials& dials = VanDials{});

// What the van is telling you, before it tells you loudly. Empty when there
// is nothing to worry about yet.
std::string VanText(const Van& van, const VanDials& dials = VanDials{});

// The worst thing about the van right now, or -1 if it is all fine.
int WorstVanPart(const Van& van, const VanDials& dials = VanDials{});

}  // namespace dirtbag

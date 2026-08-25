// The town your gym is in.
//
// `Sim/DirtbagGym.{h,cpp}` is pass one -- five levers and a daily tick --
// and every one of those levers pulled against a **static number**. Set the
// price, pick the mix, buy the mats, and the membership drifted toward a
// target that had already decided what it was going to be. There was no way
// to have a bad month you did not cause.
//
// This file is the other half of that: `GYM-4` (there are two other gyms in
// this town and they are not sitting still) and `GYM-6` (a year has a
// shape, and things go wrong in it). Ported from the 2D source.
//
// ## Why it is a separate file
//
// Everything here is a **pure function of the day and your gym's name**.
// Nothing in it takes a `Gym`, nothing in it stores state, and nothing in
// it needs a migration. That is not an accident of the port -- the original
// is built that way on purpose, so that the same save always sees the same
// town and no amount of opening and closing a panel can re-roll it. Keeping
// it free of `Gym` keeps the include one-directional: `DirtbagGym.h`
// includes this, never the other way round.
//
// ## The one deliberate deviation: the season
//
// The original keys the seasonal pull off equal calendar quarters. **This
// port does not have equal quarters** -- `SeasonName` in
// `DirtbagConditions` names the season off the temperature, which makes
// summer 121 days, winter 122, and the two shoulders 61 each.
//
// That matters because of the original's own invariant: **the four pulls
// sum to zero**, so the year is a rhythm and not a tax. Ported as written
// against this port's season lengths they average **+0.0298** -- a silent
// 3% permanent bonus to every gym in the game forever, which is exactly the
// bug the source's `GYM-4` comment records finding in the other direction.
// So the four numbers are re-centred on this port's own calendar. The
// shape is unchanged and the argument is unchanged: the slump is autumn,
// because autumn is send season and the whole town is at the crag.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

namespace dirtbag {

// The other two rooms. Both existed in the 2D game's league tables long
// before you could buy anything, which is why they have these names.
enum class GymRival { SendCity, TheCave, kGymRivalCount };
constexpr int kGymRivalCount = static_cast<int>(GymRival::kGymRivalCount);

// Named off the climate model rather than the calendar, so that "the season
// the rock is on" means the same thing here as it does everywhere else in
// this port.
enum class GymSeason { Spring, Summer, Autumn, Winter, kGymSeasonCount };
constexpr int kGymSeasonCount = static_cast<int>(GymSeason::kGymSeasonCount);

// The things that go wrong, and the one that goes right.
enum class GymIncident {
  None, Pipe, Spinner, Ac, Viral, Inspector, Poach, kGymIncidentCount
};
constexpr int kGymIncidentCount = static_cast<int>(GymIncident::kGymIncidentCount);

struct GymTownDials {
  // --- what the other two are up to (GYM-4) -----------------------------
  // A move lands this often, and fades over this many windows at this rate,
  // so the town has a memory but not a permanent one. Nine days is short
  // enough that a bad stretch is a month and not a year.
  int moveDays = 9;
  int moveMemory = 4;
  double moveDecay = 0.62;

  // The least a rival can be worth relative to itself. The source's comment
  // records why: without it, two simultaneous bad patches sent pressure
  // through the ceiling on 0.9% of days. A bad month is a bad month, not an
  // evacuation.
  double rivalFloor = 0.72;

  // **The clamp at both ends.** A rival hot streak is a squeeze rather than
  // a death sentence, and their bad one is an advantage rather than a
  // monopoly.
  double pressureMin = 0.62;
  double pressureMax = 1.50;

  // --- the year (GYM-6) -------------------------------------------------
  // Multiplied onto the target, indexed by GymSeason. Re-centred from the
  // source's {-0.04, +0.10, -0.14, +0.08} against this port's unequal
  // seasons so the year still averages zero -- see the note at the top.
  //
  // Which way it runs is the interesting part, and the obvious real-world
  // answer is wrong in this world: **the slump is autumn**, when the rock
  // is in and the whole town is outside, and the rush is summer and winter,
  // the two seasons the climate model calls off.
  double seasonPull[kGymSeasonCount] = {-0.07, 0.07, -0.17, 0.05};

  // --- and the things that go wrong (GYM-6) -----------------------------
  // Per day, once the gym is past its settling-in window. Rare enough that
  // one is an event rather than an expense line: about one every month.
  double incidentChance = 0.035;
  int incidentGrace = 14;

  // How long you have before the second option happens by itself. An
  // unanswered incident answers itself the way you would expect -- the
  // cheap way, and its cost.
  int incidentDays = 4;

  // What matching the other gym's offer for your setter costs you, per day,
  // forever. The only incident choice that is not a one-off.
  double poachRaise = 9.0;
};

// --- the town -------------------------------------------------------------

const char* GymRivalName(GymRival r);

// Which window a day falls in. Public because the readout wants to say
// whether the town's last move is new news or a fortnight old.
int TownWindow(int day, const GymTownDials& dials = GymTownDials{});

// What the town did in a given window. Which rival moved and what they did
// is derived from the window and the salt -- **the salt is your gym's
// name**, and the source records why: without it the sequence keys on the
// day alone, which makes the town's whole history one fixed script
// identical in every save, and measured over the days players actually
// reach that script sits slightly against you. Salting it makes the town
// yours.
struct TownMove {
  GymRival who = GymRival::SendCity;
  const char* what = "";
  double pull = 0.0;
};
TownMove WhatTheTownDid(int window, const std::string& salt);

// A rival's strength today: their base, plus every move they have made
// inside living memory, faded.
double RivalStrength(GymRival r, int day, const std::string& salt,
                     const GymTownDials& dials = GymTownDials{});

// **What the other two gyms are up to this month, as a multiplier on your
// target.** Normalised so that a town whose rivals are exactly at their
// base sits at 1.0 -- everything above that you earn off them, everything
// below it they take off you.
//
// `campaignShield` blunts them: that is what marketing is for. It is the
// one lever of yours that touches this number, and deliberately so -- every
// other lever is *already* in the target, and scoring them here as well
// would double-count the player's own choices. **They move the multiplier,
// you move the base.**
double TownPressure(int day, const std::string& salt,
                    double campaignShield = 1.0,
                    const GymTownDials& dials = GymTownDials{});

// --- the year -------------------------------------------------------------

GymSeason GymSeasonOf(int day);
const char* GymSeasonNote(GymSeason s);
double SeasonPull(int day, const GymTownDials& dials = GymTownDials{});

// --- and the things that go wrong -----------------------------------------

struct GymIncidentChoice {
  const char* label = "";
  double cost = 0.0;      // cash, up front
  double members = 0.0;   // and what it does to the floor
  double standing = 0.0;  // in scene points, before the /100 the caller does
  const char* line = "";
  // The one choice in the table that is not a one-off: matching the offer
  // puts your setter's wage up permanently.
  bool matchesTheOffer = false;
};

struct GymIncidentDef {
  GymIncident id = GymIncident::None;
  const char* title = "";
  const char* text = "";
  GymIncidentChoice choices[2];
};

// Null for `None` or an out-of-range id.
const GymIncidentDef* IncidentDef(GymIncident id);

// Does one land today? **Derived, not rolled** -- from the day and the
// gym's name, so it cannot double-fire and cannot be re-rolled by closing
// and reopening a panel. Only the player's *answer* is state.
//
// `hasSetter` is a parameter rather than a `Gym` because the poach only
// exists if there is somebody to poach.
GymIncident IncidentToday(const std::string& gymName, int ownedDay, int day,
                          bool hasSetter,
                          const GymTownDials& dials = GymTownDials{});

}  // namespace dirtbag

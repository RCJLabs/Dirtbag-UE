#pragma once

// The national team, which is a roster and not a threshold.
//
// **`NATL_TIERS` has read "National Team" at 700 points since the ranking
// existed, and until now it was a word on a progress bar.** The 2D game's
// own note on this is the whole design brief and is worth quoting rather
// than paraphrasing:
//
//   *"A national team is not a threshold. It's a roster with your name typed
//   on it, five other people who are also on it, a head coach who has
//   opinions about you, a stipend that doesn't cover rent, and a review at
//   the end of every domestic season that can quietly take all of it back."*
//
// So: selection reads your ranking when a circuit season closes. Clear
// `selectAt` and you are named. **Once you are on you hold all the way down
// to `holdAt`** -- the grace a selection committee actually gives a
// returning athlete -- and below that you are cut, with the door left open.
//
// **Nothing in here touches comp scoring, circuit points or the ranking
// itself.** The ladder is unchanged; it finally has a landing on it.
//
// ## Where the people come from
//
// Your teammates are **the top of the national field** -- the exact people
// you have spent a career chasing up the standings. They are not generated
// for the occasion, which is what makes being named feel like arriving
// somewhere rather than being handed a menu of strangers.
//
// A teammate is a person before they are a number, so each carries a role
// read off what they are known for and which way they are going: the
// crimper, the engine, **the junior**, **the veteran**. As the field turns
// over, the roster turns over under you for free.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagComp.h"
#include "DirtbagRng.h"

namespace dirtbag {

enum class TeamStatus {
  Never,  // never been called
  Named,  // on the paper right now
  Cut,    // you were on it and you are not
};
const char* StatusText(TeamStatus s);

struct Teammate {
  std::string name;
  std::string role;   // what they are in the room
  double points = 0.0;
};

struct NationalTeam {
  TeamStatus status = TeamStatus::Never;
  // **Getting the call once never un-happens.** Being cut takes the spot,
  // not the fact -- which is the difference between a status and a career.
  bool everNamed = false;
  // Years on the roster, cumulative. Named `seasons` from when the review
  // sat once a domestic season; it counts years now, because that is how
  // often the committee meets.
  int seasons = 0;
  int cuts = 0;      // times the review went the other way
  int namedOnDay = 0;

  // Fixed at your first call-up. You do not get a new coach for coming back.
  std::string coach;
  std::string coachKnownFor;

  std::vector<Teammate> roster;
  // Teammates who were on the paper and are not now.
  std::vector<std::string> gone;
  // **The climber you went past to get on it. They know.**
  std::string passed;

  // Your ranking at the last review, so the committee can see a direction
  // rather than only a number.
  double lastReviewPoints = 0.0;
  int lastReviewSeason = 0;
  // And when it last sat. **In days, because seasons turned out to be the
  // wrong unit** -- a domestic season is about seventy-three days, so
  // "once a season" meant five times a year. Zero means never.
  int lastReviewDay = 0;
};

struct TeamDials {
  // The line, and it is the same 700 the ranking has always called
  // "National Team". Two numbers meaning one thing would drift.
  double selectAt = 700.0;
  // **And the line where the spot goes away, which is lower.** Hysteresis,
  // because a committee does not drop somebody the week they slip below the
  // number they were picked on -- and because a threshold with no grace
  // makes a season spent hovering into a coin flip you take five times.
  double holdAt = 560.0;

  int size = 5;   // teammates on the paper alongside you

  // **How often the committee is allowed to sit.** Once a year, and this
  // is a floor on the calendar rather than a schedule -- the review still
  // runs at a circuit season's close, it is simply refused if the last one
  // was inside a year.
  //
  // Pass 3 put the review on the domestic season on the grounds that *"a
  // domestic season is the unit a selection committee actually works in"*,
  // and the reasoning was right about the unit and wrong about the length:
  // a season is about seventy-three days, so **the committee sat fifty
  // times in a ten-year career** and the team was exactly the thermostat
  // that note said it was avoiding. Measured, not argued.
  int reviewEveryDays = 365;

  // **The federation's annual stipend, and it is famously not a living.**
  // Deliberately small: this is not how a climber eats, and a number large
  // enough to matter would make the team an economic decision rather than a
  // career one.
  //
  // Was $180 "per season" and paid five times a year, because the review
  // sat five times a year. This is the same money on an honest label.
  double stipend = 900.0;

  // What the news is worth **in standing units**, where
  // `Sim/DirtbagFactions.h` says *"0.1 is a small deliberate act, 0.3 is a
  // big one"*.
  //
  // **The 2D game's numbers here are 16, 6 and 6, and they are not these
  // units** -- they are its own `rep` points, on a scale that runs to
  // hundreds. Carrying them across unconverted would have shifted the
  // Scene by sixteen on a scale where one is the whole range, which is the
  // rival's grade-for-skill bug wearing a different hat: the same mistake
  // is available every time a number crosses between two systems that both
  // call their axis "reputation".
  //
  // 0.4 is deliberately **above** what the factions file calls a big
  // deliberate act, because being named to the national team is the largest
  // single thing that can happen to a competitor's standing with the scene.
  // Coming back and being dropped are both 0.15 -- real, and not the same
  // size as arriving.
  double namedRep = 0.40;
  double renamedRep = 0.15;
  double cutRep = 0.15;
};

// The four of them. Deterministic from the season you were called up in.
struct Coach {
  const char* name;
  const char* knownFor;
};
const std::vector<Coach>& TheCoaches();
Coach CoachFor(int season);

// Build the roster from the season's standings: the top of the field, each
// with a role read off what they are known for.
std::vector<Teammate> RosterFrom(const Circuit& circuit,
                                 const TeamDials& dials = TeamDials{});

// What happened at the review.
struct TeamReview {
  bool changed = false;
  TeamStatus was = TeamStatus::Never;
  TeamStatus now = TeamStatus::Never;
  double stipend = 0.0;
  double rep = 0.0;     // signed: negative when you are dropped
  std::string news;     // said once, in the game's voice
};

// The review, at a circuit season's close. **This is the only thing that
// changes team status** -- not a comp, not a day, not a grade. A domestic
// season is the unit a selection committee actually works in.
TeamReview ReviewTheTeam(NationalTeam& team, double rankingPoints,
                         const Circuit& circuit, int day, int season,
                         const TeamDials& dials = TeamDials{});

// One line for the HUD. Empty until you have ever been called, because a
// team you have never been near is not a status.
std::string TeamLine(const NationalTeam& team,
                     const TeamDials& dials = TeamDials{});

}  // namespace dirtbag

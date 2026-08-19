// Factions: the scene, and the fact that it remembers.
//
// Four groups with genuinely opposed values, arranged on two axes so that
// pleasing one costs another. A faction system where you can max everything
// is a checklist; this one makes you pick.
//
//   Old Guard  <-->  The Scene
//     ground-up, good style, the grade is the grade, nobody needs to know
//     against comps, sponsors, media, and the crag being famous
//
//   Development  <-->  Stewardship
//     new lines, scrubbed clean, more terrain for everyone
//     against low impact, quiet approaches, and the crag still being open
//
// Almost everything that moves standing is something the player already
// does for other reasons, which is the point: the best-paying gig on the
// board is shooting photos for the guidebook, and it costs you the Old
// Guard and Stewardship both. Money against reputation is the pressure
// Phase 3 is supposed to be about, and this is where it stops being only
// about money.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

enum class Faction {
  OldGuard,      // first ascensionists, ethics, ground-up
  Scene,         // comps, sponsors, media, the gym
  Development,   // new lines, cleaning, opening terrain
  Stewardship,   // access, low impact, keeping the crag open
};
constexpr int kFactionCount = 4;

const char* FactionName(Faction f);

// Who each faction is set against. Gaining with one costs a little with its
// opposite, which is what "opposed" has to mean mechanically or it means
// nothing.
Faction OppositeOf(Faction f);

struct FactionDials {
  // What a gain with one faction costs its opposite. Under half, so a
  // career can lean without being forced into a corner.
  double oppositionBleed = 0.4;

  // Standing runs -1 (they would rather you left) to +1 (you are one of
  // theirs). It drifts back toward nothing, slowly: the scene remembers,
  // but not forever, and not as clearly as you think.
  double driftPerDay = 0.0012;

  // Below this, Stewardship starts closing the crag. Not out of spite —
  // access gets pulled when the landowner has had enough, and you were part
  // of that.
  double closureBelow = -0.45;
  double closureChancePerDay = 0.06;
  int closureDays = 9;

  // What standing is worth in beta from the people who share those values.
  // Small: a faction is a relationship, not a stat block.
  double betaBonusAtFullStanding = 0.25;
};

struct Standing {
  double with[kFactionCount] = {0.0, 0.0, 0.0, 0.0};

  // Days left on an access closure, if the crag is shut.
  int closedDays = 0;
};

double StandingWith(const Standing& s, Faction f);

// Move standing, and bleed the opposite. `amount` is in standing units;
// 0.1 is a small deliberate act, 0.3 is a big one.
void Shift(Standing& s, Faction f, double amount,
           const FactionDials& dials = FactionDials{});

// The things the player already does, priced. One place, so the values are
// visible together rather than scattered across the systems that trigger
// them.
void DidFirstAscent(Standing& s, bool goodStyle,
                    const FactionDials& dials = FactionDials{});
void ScrubbedALine(Standing& s, const FactionDials& dials = FactionDials{});
void TookTheGuidebookPhotos(Standing& s,
                            const FactionDials& dials = FactionDials{});
void DidTrailWork(Standing& s, const FactionDials& dials = FactionDials{});
void SetAtTheGym(Standing& s, const FactionDials& dials = FactionDials{});

// A day passes: standing drifts toward nothing, a closure counts down, and
// a crag with the landowner against it may shut.
void FactionDay(Standing& s, const Rng& worldRng, int day,
                const FactionDials& dials = FactionDials{});

bool CragIsOpen(const Standing& s);

// Which faction someone at the Lot speaks for, so that standing means
// something in a conversation rather than only on a screen.
Faction FactionOf(const std::string& partnerName);

// How much more (or less) beta somebody gives you for who you are.
double BetaMultiplierFor(const Standing& s, const std::string& partnerName,
                         const FactionDials& dials = FactionDials{});

// "the old guard have time for you" / "the rangers know your van"
std::string StandingText(const Standing& s);

}  // namespace dirtbag

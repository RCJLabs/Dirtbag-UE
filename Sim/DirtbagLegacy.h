// Retiring, and what a career leaves behind.
//
// "Retire eventually — legacy tallied, next generation inherits"
// (concepts/DIRTBAG.md). Aging exists now, so a career can decline; without
// this it cannot end, and a climber who cannot stop is not a career, it is
// a treadmill.
//
// The design call, and the whole of what makes this worth building:
//
//     **The world remembers. The body does not.**
//
// Nothing physical carries over. The next climber is twenty-four with a
// fresh body, no skills to speak of, and a van — because inheriting
// somebody else's fingers would be nonsense and would make the second life
// a save-scum of the first. What carries is everything *outside* the body:
// the lines you put up are in the guidebook under the names you gave them,
// the Lot remembers who you were, and the crag is open or shut because of
// what you did about it.
//
// That is the version of legacy this game can actually mean. A first ascent
// only becomes permanent when somebody else has to climb past your name.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagCrag.h"
#include "DirtbagDay.h"
#include "DirtbagDreams.h"
#include "DirtbagFactions.h"

namespace dirtbag {

struct LegacyDials {
  // Nobody is thrown out. Retirement is offered, never forced — the game
  // says where you are and the player decides, because deciding when to
  // stop is the last real choice a climbing career contains and taking it
  // away would be the one unforgivable thing to do to it.
  //
  // These only decide when the game starts being honest with you about it.
  double retirementAgeHint = 46.0;

  // Two grades off your best, sustained, is the sign — not age. Plenty of
  // people climb their hardest at forty; nobody climbs their hardest two
  // grades below their hardest.
  double gradesOffPeakToHint = 2.0;

  // A body that keeps breaking says it before the numbers do.
  int injuriesInARowToHint = 3;

  // What the next one starts with. Not gear, and certainly not skills: the
  // van, and whatever was in the coffee tin. A career that ended rich
  // should not hand the next one a shortcut past the part of the game that
  // is about being broke.
  double inheritedCash = 300.0;

  // Standing you inherit as a fraction of what was earned. You are not
  // your predecessor, but the Lot knows whose van that is — and being
  // somebody's kid brother cuts both ways, which is why this is signed.
  double inheritedStandingShare = 0.35;
};

// One line somebody put up, as it stands in the book forever after.
struct NamedLine {
  std::string routeKey;     // the ledger key, never the given name
  std::string givenName;    // what they called it
  int confirmedGrade = -1;  // what it turned out to be
  Style style = Style::Fell;
  Discipline discipline = Discipline::Boulder;   // which ladder to print
  std::string by;           // who put it up
};

// What a career amounts to, once it is over.
struct Legacy {
  std::string name;             // whose it was
  int seasons = 0;
  double retiredAt = 0.0;       // age
  int hardestSendGrade = -1;
  std::string hardestSendName;
  Discipline hardestSendDiscipline = Discipline::Boulder;
  int totalSends = 0;
  int totalAttempts = 0;
  std::vector<NamedLine> firstAscents;
  Standing standing;
  bool cragOpenAtTheEnd = true;
  std::string nemesis;          // the one that never went
  int nemesisAttempts = 0;

  // Whole years lived without the nine-to-five, and the longest unbroken
  // run in days. Both, because they answer different questions: how many
  // times you did it, and whether the one you did not finish was close.
  int dirtbagYears = 0;
  int longestDirtbagStreak = 0;

  // What the town called them. Outlives the crew and the career both --
  // that is what a nickname does.
  std::string crewName;

  // And what the money was for, in the end. Kept whole rather than as a
  // count because which ones matter: a career that got the Rig and never a
  // roof is a different life from one that got the roof and stayed
  // stranded.
  Dreams dreams;
};

// Should the game start being honest about stopping? Never a command.
bool TimeToThinkAboutIt(const PlayerState& player, int consecutiveInjuries,
                        double peakGradeEver,
                        const LegacyDials& dials = LegacyDials{});

// Everything the career was, frozen. Takes the crag so a first ascent can
// record what the line actually went at rather than what anyone guessed.
Legacy TallyCareer(const PlayerState& player, const std::string& name,
                   int seasons, const LegacyDials& dials = LegacyDials{});

// The next one. Twenty-four, nothing in the fingers, and a valley that
// already has your predecessor's name written on it in three places.
//
// Takes the world because the next climber is drawn from it, salted with
// whose career just ended -- so generation two of the same world is a
// different body from generation one, deterministically, and a reload
// hands you back the same successor.
PlayerState Inherit(const Legacy& previous, const Rng& worldRng,
                    const LegacyDials& dials = LegacyDials{});

// "Nine seasons. Hardest: The Guidebook Lied, V7. Four lines that are yours
// now. Retired at 47, with the crag open."
std::string LegacyText(const Legacy& legacy);

// What the guidebook says about a line once somebody has named it — the
// entry a later climber reads without ever meeting them.
std::string GuidebookEntry(const NamedLine& line);

// --- Who turns up ------------------------------------------------------------
//
// Three names for the climber who arrives after you, drawn from the world
// and the generation, so a reload offers the same three and generation four
// is offered different ones from generation two.
//
// This exists because the engine had **no way to name a climber at all.**
// `ClimberName` was an EditAnywhere string on the game instance with no in-
// game setter anywhere, so every career signed its first ascents "you", and
// the crew hash -- which was given the player's name specifically so four
// generations would stop being one generation four times -- was being fed an
// empty string.
//
// A pick rather than a text box, deliberately. Typing needs a widget, an
// editor and a keyboard focus fight; picking needs three keys the player
// already uses for the dream and the stake. And it reads better: you are not
// naming yourself, **you are meeting the person who rolled into the Lot**,
// which is what a handover actually is.
constexpr int kNameChoices = 3;

// `generation` is 0 for the first career, 1 for the first inheritor, and so
// on -- so the same world does not offer the same three names forever.
std::vector<std::string> ThreeWhoCouldTurnUp(const Rng& worldRng,
                                             int generation);

}  // namespace dirtbag

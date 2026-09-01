#pragma once

// Somebody to beat.
//
// **`concepts/DIRTBAG.md` §4 ports "projecting/nemesis tracking", and that
// is the route, not the person.** `DirtbagDay.h`'s `nemesis` is the unsent
// line you have fed the most burns, it has been built for weeks, and it is a
// different thing entirely. The 2D game also has a **rival**: a named
// climber who was already here when you arrived, whose grade tracks a step
// ahead of yours, who **takes first ascents of open lines and keeps their
// name on them forever**, who ages on their own clock, who eventually
// retires into the scene rather than out of it -- and who is replaced,
// because somebody always steps up.
//
// The port has partners, factions, a crew, a scene and a guidebook. It has
// nobody to lose to. That is the whole of what this file is for.
//
// ## Two things this deliberately does not duplicate
//
// **Taking a line is the Lot's machinery.** `TheyPutUpTheLine(line, who)` is
// keyed on a *name* rather than a `Partner`, which was written that way for
// the Lot's regulars and turns out to be exactly what a rival needs. A rival
// who took a line goes into the guidebook by the same path a neighbour does,
// so the book cannot learn about one and not the other.
//
// **Ageing is `DirtbagAge`'s model, not a second one.** A rival peaks and
// declines because that is what the player does; two curves that mean
// "getting older" would drift, and the drift would be invisible until
// somebody noticed the rival was thirty-nine and still improving.
//
// ## What moves the rivalry
//
// In the 2D game the head-to-head number is mostly comps, and **comps are
// Phase 9.** So here it is first ascents, which is the currency this build
// already has: you get to an open line first and it goes up, they get there
// first and it goes down. That is not a placeholder -- an FA is the highest
// stake this game currently has, and losing one is permanent.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagPartner.h"
#include "DirtbagRng.h"

namespace dirtbag {

// What kind of rival they are. Rolled, and it decides how they talk, how the
// lead changes read, and what becomes of them at the end.
enum class RivalVibe {
  Foil,       // friendly about it. Beats you and buys you a coffee.
  Nemesis,    // not friendly about it.
  Benchmark,  // barely acknowledges you. Somehow the worst one.
};
constexpr int kRivalVibeCount = 3;
const char* VibeText(RivalVibe v);

// What they do afterwards. **They do not leave the world**, which is the
// entire point of ageing them: the scene keeps moving whether or not you do.
enum class RivalRole {
  Coach,   // the other side of the gym counter, and they know every weakness
  Author,  // their name on the guidebook that put your quiet crag on the map
  Gone,    // you hear about them once in a while
};
const char* RoleText(RivalRole r);

// A line with a deadline on it.
//
// **This is the answer to Phase 8's first gate.** A rival who only takes
// lines removes them from the world; measured, a career's first ascents came
// out at 1.38 with or without one, because nothing about them made you
// *choose* differently. A race does: they are on a named line, you have
// until a stated day, and everything else you might have climbed this week
// is now a decision rather than a default.
struct Race {
  std::string routeName;   // empty means no race is on
  int byDay = 0;           // the day they finish it if you have not
  bool forFirstAscent = false;  // an open line, which is the version that
                                // cannot be undone
};

struct Rival {
  std::string name;
  RouteType style = RouteType::Power;  // what they are best at
  RivalVibe vibe = RivalVibe::Benchmark;

  int generation = 0;   // 0 is the one who was here when you arrived
  int bornOnDay = 1;    // their clock, which is not yours
  double startAge = 26.0;

  double grade = 0.0;   // where they are on the ladder today
  int lastStepDay = 0;  // and when they last moved up it
  double peakGrade = 0.0;

  // Net head-to-head. Positive is you. Reaching the ally threshold is what
  // makes them offer to rope up; it is not a friendship meter and it can go
  // the other way for a whole career.
  double rivalry = 0.0;
  bool allied = false;
  bool offered = false;   // the offer has fired, accepted or not
  bool met = false;       // before this they are a name and a number

  // Open lines they got to before you. Their name is on them in the
  // guidebook forever; this is the career-level record of it.
  std::vector<std::string> firstAscents;

  bool retired = false;

  // What they are on right now, if anything.
  Race race;
};

// The ones who came before, and what became of them.
struct PastRival {
  std::string name;
  RivalRole role = RivalRole::Gone;
  RouteType style = RouteType::Power;
  int retiredOnDay = 0;
  double age = 0.0;
  double peakGrade = 0.0;
  int generation = 0;
};

struct RivalDials {
  // How far ahead they aim to stay, in grades.
  //
  // **One, and one is the whole design.** Two makes them scenery you will
  // never catch; zero makes them a mirror. One is close enough that every
  // grade you gain is a real question about whether you have caught them,
  // and far enough that the answer is usually no.
  double lead = 1.0;

  // The soonest they can move up a grade-step. Two days in the 2D game, and
  // kept: it is fast, and it is meant to be -- a rival who improves slower
  // than you is not a rival, they are a milestone you passed.
  int stepEveryDays = 2;

  // How much of a grade a step is. A tenth, so the chase is continuous
  // rather than a number that jumps once a fortnight.
  double stepSize = 0.1;

  // They never touch the mythical grades. **The top of the ladder is
  // yours**, and this is what guarantees it: a rival who could reach V18
  // would eventually take the one thing a career is for.
  double gradeCap = 17.0;

  // How old they are when you arrive.
  //
  // **Deliberately not `AgeDials::startAge`, and renamed so nothing thinks
  // it is.** The player starts at 24 and the rival at 26, and that two-year
  // gap is *why* they are the benchmark: their lead is a head start rather
  // than talent, which is what makes catching them a matter of time rather
  // than a matter of being better than them. The dial checker flagged the
  // two as one drifted number the moment they shared a name, and it was
  // right to -- a reader has no way to tell a deliberate difference from an
  // accidental one when the fields are called the same thing.
  double rivalStartAge = 26.0;
  int daysPerYear = 365;   // mirrors AgeDials; see the note there

  // After this their grade stops climbing. They are not getting better any
  // more, and if you are still improving this is the year you go past them
  // -- which is the moment the whole system exists to produce.
  double peakAge = 31.0;

  // And around here they start thinking about it.
  double retireAge = 34.0;
  // Per season, once past the age. Sized so that a long career watches it
  // happen rather than hearing about it second-hand.
  double retireChancePerSeason = 0.28;
  int seasonDays = 91;

  // The successor starts this far below *you* ...
  double successorLag = 3.0;
  // ... and climbs about twice as fast, because they always do.
  int successorStepEveryDays = 1;

  // Net head-to-head at which they come around and offer to rope up.
  double allyAt = 6.0;
  // What one first ascent is worth to it, either way.
  double faSwing = 1.0;

  // --- the race -------------------------------------------------------
  // How long you get. **Five days, and the five is the whole mechanic**: a
  // fortnight is a background hum you would have got round to anyway, and
  // two days is a coin toss the weather decides. Five is one bad-weather
  // week away from impossible, which is exactly the pressure wanted.
  int raceDays = 5;
  // Chance per morning that one starts, when none is running.
  double raceChancePerDay = 0.10;
  // No races until you are climbing real grades -- being raced for a V2 in
  // your first season is the game picking on you.
  double raceMinGrade = 3.0;
  // Of the races that start, this share are for an **open line**. Those are
  // the ones that cannot be undone, which is why they are the minority.
  double raceForFaChance = 0.40;
  // What winning and losing move the head-to-head by. Losing a repeat is a
  // shrug; losing a first ascent is the thing you remember.
  double raceWinSwing = 3.0;
  double raceLoseSwing = 2.0;
  double raceLoseFaSwing = 5.0;
};

// Roll one. `yours` leans their style toward **whatever you are weakest at**
// -- they are built to exploit what you are soft on rather than being a
// flavour generator. Name and vibe are pure chance.
Rival RollRival(const Rng& worldRng, const Skills& yours, int day,
                int generation, double yourGrade,
                const RivalDials& dials = RivalDials{});

// How old they are today. Their clock, not yours.
double RivalAge(const Rival& r, int day, const RivalDials& dials = RivalDials{});

// Are they still getting better?
bool CanImprove(const Rival& r, int day, const RivalDials& dials = RivalDials{});

// A day of their career. They chase your grade if they still can, and they
// never pass the cap. Returns true if they moved up today, so the caller can
// say so once.
bool RivalDay(Rival& r, double yourGrade, int day,
              const RivalDials& dials = RivalDials{});

// Are they ahead of you right now? The question the HUD asks every day.
bool AreTheyAhead(const Rival& r, double yourGrade);

// Do they hang it up this season? Rolled once a season on its own stream, so
// the rest of the world cannot shift it and it cannot shift the world.
bool ThinkingAboutIt(const Rival& r, const Rng& worldRng, int day,
                     const RivalDials& dials = RivalDials{});

// What becomes of them. **Allies stay close; somebody who beat you and left
// is the one that stings.**
PastRival Retire(const Rival& r, const Rng& worldRng, int day,
                 const RivalDials& dials = RivalDials{});

// Somebody steps up. **Sometimes it is a kid from the gym** -- that
// sentence has been here since long before there was a gym, and `GYM-8`
// finally makes it true: pass a graduate's name and the successor is
// somebody you coached from eleven. Empty means a stranger, which is what
// it always was.
Rival Succeed(const Rng& worldRng, const Skills& yours, double yourGrade,
              int day, int generation,
              const std::string& theyAlreadyHaveAName = std::string(),
              const RivalDials& dials = RivalDials{});

// Is one on right now?
bool RaceIsOn(const Rival& r);

// Start one, if the conditions are right and the dice agree. Picks a line
// they can do and you might: an open project when the roll says so and one
// is in reach, otherwise something already in the book. Returns true if a
// race began, and leaves `r.race` untouched otherwise.
bool StartARace(Rival& r, const Crag& crag, double yourGrade,
                const std::vector<std::string>& spokenFor, const Rng& worldRng,
                int day, const RivalDials& dials = RivalDials{});

// Has the clock run out? True on the day they finish it, and after.
bool RaceRanOut(const Rival& r, int day);

// You got there first. Clears the race and swings the head-to-head.
void YouWonTheRace(Rival& r, const RivalDials& dials = RivalDials{});

// They did. Swings it the other way -- much harder for an open line, which
// is gone rather than merely climbed by somebody else first.
void TheyWonTheRace(Rival& r, const RivalDials& dials = RivalDials{});

// What the race says, in the game's voice, with the days left in words
// rather than a number. Empty when nothing is on.
std::string RaceLine(const Rival& r, int day);

// They got to a line before you did. Records it and moves the rivalry.
void TheyGotThereFirst(Rival& r, const std::string& routeName,
                       const RivalDials& dials = RivalDials{});

// And you got to one before them.
void YouGotThereFirst(Rival& r, const RivalDials& dials = RivalDials{});

// Would they rope up with you? True once the head-to-head says you have
// earned it and they have not already asked.
bool WouldPartnerUp(const Rival& r, const RivalDials& dials = RivalDials{});

// The rival as somebody who climbs, so that **taking a line runs through
// the Lot's own machinery rather than a second roll of its own.**
// `PartnerTakesFirstAscent` and `TheyPutUpTheLine` were written for the
// neighbours and are keyed on a name; a rival is a climber with a name and a
// grade, so they go into the guidebook by exactly the path a neighbour does.
// Two rolls for "somebody got there first" would drift, and the drift would
// show up as the book knowing about one and not the other.
//
// Ambition is high and fixed: the whole of what a rival is, is somebody who
// goes for the hard ones.
Partner AsAClimber(const Rival& r, const Rng& worldRng, int day,
                   const RivalDials& dials = RivalDials{});

// One line for the HUD, in the game's voice. Empty before you have met them,
// because a number for somebody you have never been introduced to is a
// leaderboard rather than a rival.
std::string RivalLine(const Rival& r, double yourGrade, int day,
                      const RivalDials& dials = RivalDials{});

// What they are known for, said the way somebody at the fire would say it.
const char* StyleText(RouteType t);

}  // namespace dirtbag

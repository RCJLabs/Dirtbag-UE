// Habits, and the quirks they harden into.
//
// Phase 7 built who you *chose* to be — an origin, an archetype, a flaw, a
// temperament — and rolled two talents behind your back. It deliberately
// left this out and said why: *"quirks, habits and personality drift are
// all earned from a tally of how you actually climb, which needs
// instrumentation in every session and is a coherent second pass."*
//
// This is that pass, and it turns on one rule:
//
//   **A habit is what you have been doing lately, and it can change.
//    A quirk is what you turned out to be, and it does not.**
//
// Everything in this file is downstream of that. A habit is *derived* from
// a decaying record of your climbing and is never stored as a fact —
// stop doing the thing and it goes, which is what makes it a habit rather
// than a badge. A quirk is stored, permanent, and earned exactly one way:
// by holding the habit long enough that it stopped being something you were
// doing and became something you are.
//
// The trap is the point. The Grinder who hardens into one keeps the beta
// bonus for the rest of their career **and keeps the head penalty too**,
// long after they have stopped grinding — because that is what a quirk is.
// You do not get to un-become somebody.
//
// **Nothing here touches send odds.** Phase 7's rule is that only a flaw
// may, so that where you came from can never be a difficulty setting, and
// an earned trait has even less business there than a picked one: a habit
// that made the moves easier would be a game telling you how to play. These
// bend what a session *teaches*, what it *costs*, what it *risks* and how
// steady you are above the last piece — all seams that already exist and
// are already measured.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCharacter.h"
#include "DirtbagCore.h"

namespace dirtbag {

// ---------------------------------------------------------------------
// What you did
// ---------------------------------------------------------------------

// The counters. Every one of them feeds at least one habit below — a tally
// nothing reads is a save field that costs a migration and buys nothing.
enum class Did {
  Burn = 0,        // a go, any go: the denominator for most of this
  BurnAtYourLimit, // on something reading AtYourLimit or harder
  BurnOnOneLine,   // on the line you have already fed the most burns
  BurnOnThinSkin,  // started with your tips already gone
  LineTouched,     // first burn of the day on a line you had not been on
  DayOut,          // a day you actually got on something: the denominator
  DayIndoors,      // ...and it was plastic
  DawnStart,       // pulled on before the sun got to the rock

  // **Went home with something left.** Not "walked up and did not get on":
  // the first version of this counted a session that produced no burns,
  // and measured over thirty years that turned out to mean *the rock was
  // never in condition* -- one career's last season came out 71 days out
  // and **zero burns, every one of them counted as bailing**, so every
  // career in the game ended up cautious and the weather is what made
  // them that way.
  //
  // Stopping while you still have skin is a decision. Not getting on
  // because it rained for a fortnight is a fortnight.
  StoppedEarly,
  MoveOnGear,      // a move climbed on a trad lead
  MoveRunOut,      // ...with nothing on the rope at all
  PiecePlaced,     // gear that went in
  kDidCount
};
constexpr int kDidCount = static_cast<int>(Did::kDidCount);

struct HabitDials {
  // **How long "lately" is**, in days, as the half-life of the record.
  //
  // A season, roughly. Short enough that a winter indoors reads as a winter
  // indoors rather than as a career, and long enough that a fortnight of
  // rain does not rewrite who you are.
  double remembersDays = 120.0;

  // How much you have to have done before the record means anything. Two
  // burns in one lane is not a habit, it is a Tuesday — and without a floor
  // every brand-new climber would qualify for whichever habit their first
  // afternoon happened to resemble.
  double enoughBurns = 20.0;
  double enoughDays = 14.0;

  // Where a ratio becomes a habit. All of these are fractions of the
  // relevant denominator, and they are deliberately high: a habit is
  // supposed to be a thing somebody would say about you, not a thing that
  // is technically true of everyone.
  double dawnPatrolAt = 0.55;      // of days out
  double grinderAt = 0.60;         // of burns, on one line
  double touristAt = 0.42;         // of burns are a line's first
  double neverWarmsUpAt = 0.55;    // of burns at your limit or above
  double gymRatAt = 0.62;          // of days out, indoors
  double skinOfSteelAt = 0.35;     // of burns on skin already gone
  double knowsWhenToStopAt = 0.30; // of days out, ended with skin left
  double runsItOutAt = 0.30;       // of trad moves with nothing on the rope
  double sewsItUpAt = 0.34;        // pieces per trad move

  // How many days a habit has to hold before it is who you are.
  //
  // Two seasons. Long enough that it is a phase of a career rather than a
  // good month, and short enough that a thirty-year career hardens several
  // — which is the shape wanted, because a climber who ends up with one
  // quirk is a climber the system barely touched.
  double quirkAfterDays = 180.0;

  // ...and how fast it drains when you stop. Slower than it fills, because
  // nearly becoming something and then not is a real thing that happens and
  // should cost you the progress rather than the memory of it.
  double quirkDrainRate = 0.55;

  // --- What they do -----------------------------------------------------
  //
  // All small. A habit is flavour with teeth, not a build — the archetype
  // table is where a career-shaping choice belongs, and it sums to zero on
  // purpose. These do not sum to anything, because what you did is not a
  // choice you were offered.

  // Skill-lane multipliers, applied on top of everything Phase 7 already
  // applies. 1.18 is about what a good origin's perk is worth.
  double lanePush = 1.18;
  double laneStarve = 0.86;

  // Injury risk, multiplied.
  double riskPush = 1.22;
  double riskEase = 0.86;

  // What the Grinder and the Tourist do to how fast a line's beta comes.
  double betaPush = 1.45;
  double betaStarve = 0.72;

  // What Skin of Steel takes off a burn's skin cost. It does not make skin
  // free; it makes you somebody who is used to it.
  double skinEase = 0.82;

  // Nerve, on the resolver's 0..1 scale, same units as the temperament's.
  double nerveBold = 0.10;
  double nerveShy = -0.08;

  // A quirk is the habit made permanent, and slightly more so — you have
  // been doing it for two seasons, it has got into your hands.
  double quirkStrength = 1.25;
};

// The logbook. Counters decay toward nothing on a half-life, so this is
// "lately" without storing three months of dated entries.
//
// Named for the book rather than for the data because `Record` was taken --
// `DirtbagComp.h` has a *function* by that name, which the unity build
// found in about four seconds. Third name collision in a fortnight, and the
// cheapest one to have caught.
//
// Decayed *by the date* on every read and every write rather than by
// repeated multiplication somewhere — the same discipline the ranking and
// the scars arrived at, both of which shipped a compounding bug first.
struct Logbook {
  double count[kDidCount] = {0};
  int asOfDay = 0;

  // Never decays. What a career did, for the guidebook and for the "have
  // you done enough for any of this to mean anything" floor.
  double lifetimeBurns = 0.0;
  double lifetimeDays = 0.0;
};

// Fold the record forward to today. Called by Note, and separately by the
// night tick so that a month off actually reads as a month off rather than
// waiting for the next burn to notice.
void RememberTo(Logbook& record, int day, const HabitDials& dials = HabitDials{});

// Count something. `amount` so a session can hand over twenty moves in one
// call rather than twenty calls.
void Note(Logbook& record, Did what, double amount, int day,
          const HabitDials& dials = HabitDials{});

// ---------------------------------------------------------------------
// What that makes you
// ---------------------------------------------------------------------

enum class Habit {
  None = 0,
  DawnPatrol,     // up before the sun is on it, most days
  Grinder,        // one line, and one line, and one line
  Tourist,        // a burn on everything and a second burn on nothing
  NeverWarmsUp,   // straight onto the hard one
  GymRat,         // most of your days are plastic
  SkinOfSteel,    // you climb on tips that are already gone
  KnowsWhenToStop,  // you go home with something in the tank
  RunsItOut,      // a long way above the last piece, by choice
  SewsItUp,       // and the other kind of leader
  kHabitCount
};
constexpr int kHabitCount = static_cast<int>(Habit::kHabitCount);

const char* HabitName(Habit h);

// What the game says about it, second-person and dry. One line.
const char* HabitLine(Habit h);

// Is this how you have been climbing? False for everything until you have
// done enough for the question to mean anything.
bool Doing(const Logbook& record, Habit h, int today,
           const HabitDials& dials = HabitDials{});

// Everything true of you right now, loudest first. Usually one or two —
// most of these are ratios of the same denominator and cannot both be high.
std::vector<Habit> HabitsNow(const Logbook& record, int today,
                             const HabitDials& dials = HabitDials{});

// ---------------------------------------------------------------------
// And what you turned out to be
// ---------------------------------------------------------------------

// One per habit, plus the ones you pick at the start. Picked and earned
// share an enum on purpose: by the time anybody is reading it back, *how*
// you came to be like this is not the interesting part.
enum class Quirk {
  None = 0,

  // Earned, in the same order as the habits they harden from.
  MorningPerson,
  Obsessive,
  Magpie,
  Impatient,
  PlasticMerchant,
  Leathery,
  Cautious,
  Bold,
  Fastidious,

  // Picked, at the start, and not available any other way. Each is a small
  // perk against a small cost, because a quirk you choose has to be a
  // decision and not a bonus.
  LightSleeper,   // you are up anyway; the mornings are free and the days are long
  BadWithMoney,   // things cost you more and you never once minded
  Superstitious,  // a line you have sent is a line you trust
  Stubborn,       // you do not walk away, for better and for worse
  Gregarious,     // the Lot likes you and the work does not
  Quiet,          // nobody watches you climb, and nobody has to

  kQuirkCount
};
constexpr int kQuirkCount = static_cast<int>(Quirk::kQuirkCount);

const char* QuirkName(Quirk q);
const char* QuirkLine(Quirk q);

// Which quirk this habit hardens into, and the way back. `None` for the
// picked ones, which no habit produces.
Quirk HardensInto(Habit h);
Habit HabitBehind(Quirk q);
bool IsPicked(Quirk q);

// What has stuck, and how close the rest are.
struct Quirks {
  // Permanent. Order is the order they landed, which is a small biography.
  std::vector<Quirk> held;

  // Days each habit has held, drained when it lapses. The only mutable
  // part, and the reason a habit has to *keep* being true rather than be
  // true once on the right afternoon.
  double heldFor[kHabitCount] = {0};

  // What you picked at the start, if anything. Kept separately from `held`
  // so that "what did you choose to be" survives a career of becoming
  // things — and it goes into `held` too, because by the time anything is
  // reading it back the distinction has stopped mattering.
  Quirk picked = Quirk::None;
};

bool Has(const Quirks& q, Quirk which);

// The night tick. Advances every habit you are currently holding, drains
// the rest, and returns the quirk that just landed — or `None`, so a caller
// can say it out loud once without asking twice.
Quirk HabitsDay(Quirks& quirks, const Logbook& record, int today,
                const HabitDials& dials = HabitDials{});

// The line the game says on the day a habit stops being a habit. Empty for
// `None`.
std::string QuirkLanded(Quirk q);

// Give somebody the one they chose. Idempotent and refuses an earned quirk,
// because picking Obsessive at the counter is not the same thing as
// becoming it and this file's whole rule is that the difference matters.
bool Pick(Quirks& quirks, Quirk which);

// ---------------------------------------------------------------------
// What it all does
// ---------------------------------------------------------------------
//
// Same shape as DirtbagCharacter.h's effect block, and for the same reason:
// every one of these is a multiplier on a number some other module already
// computes, so this file stays additive and nothing has to be rewritten to
// let it in. A quirk applies whether or not you are still doing the thing;
// a habit applies only while you are.

double HabitSkillGain(const Quirks& quirks, const Logbook& record, Skill lane,
                      int today, const HabitDials& dials = HabitDials{});
double HabitInjuryRisk(const Quirks& quirks, const Logbook& record, int today,
                       const HabitDials& dials = HabitDials{});
double HabitBetaRate(const Quirks& quirks, const Logbook& record, int today,
                     const HabitDials& dials = HabitDials{});
double HabitSkinCost(const Quirks& quirks, const Logbook& record, int today,
                     const HabitDials& dials = HabitDials{});
double HabitNerve(const Quirks& quirks, const Logbook& record, int today,
                  const HabitDials& dials = HabitDials{});
double HabitShiftPay(const Quirks& quirks);
double HabitDailyCost(const Quirks& quirks);

// One sentence, in the game's voice, about how you have been climbing —
// and what you have become, which are different questions and are answered
// separately on purpose.
std::string HowYouClimb(const Quirks& quirks, const Logbook& record, int today,
                        const HabitDials& dials = HabitDials{});

}  // namespace dirtbag

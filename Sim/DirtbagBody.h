// Training load, and what it does to you.
//
// "Your body is the save file" (concepts/DIRTBAG.md): skin, pump, training
// load, injury, age. Skin and pump were built in Phase 0-2. This is the
// third, and it is the one two Phase 3 measurements kept running into.
//
// Both of those ended at the same wall. Skin caps the climbing year, so
// anything that spends skin can only move a season around rather than add
// to it — which is why the gym and the hangboard measured neutral while a
// crash pad, which spends no skin at all, was worth +37% sends
// (notes/phase3-kit.md). The conclusion there was that money needs
// something to buy that is not more skin.
//
// Load is that something, from the other side. It is a budget you cannot
// buy your way out of by climbing more, only by resting or by paying a
// physio — and it is what finally makes a rest day a decision rather than
// a wait.
//
// The model, in one line: **hard climbing accrues load, load heals slowly,
// and load past a threshold is how you get hurt.**
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagAge.h"
#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

struct BodyDials {
  // --- Accrual ----------------------------------------------------------
  // How hard you pull, not how much you climb. Load takes the *same*
  // challenge number the trainer uses — two ways of asking "was that hard"
  // would drift, and the drift would be invisible until somebody got hurt
  // for no reason. At your level that number is about 0.67, two grades
  // below it is 0, above it is 1.
  //
  // The first pass at this asked "how many grades over you was it", which
  // is a different question and the wrong one: a player projecting at their
  // limit is over by roughly zero, so a whole season peaked at a load of 6
  // out of 100 and nobody was ever going to get hurt. Climbing at your
  // limit *is* the maximal intensity — that is what limit means.
  // A burn at full challenge, on fingery holds. Sized against recovery
  // rather than guessed: a four-burn session has to take three or four
  // nights to clear, or load can never accumulate across a block and the
  // whole budget is decoration. At 1.6 a season peaked at 2 out of 100.
  double loadPerAttempt = 3.0;
  double loadCeiling = 100.0;

  // Crimps and pockets are what hurt; a day on slopers is nearly free.
  // This is why a shoulder injury does not stop you crimping and a pulley
  // does not stop you on slopers — the same asymmetry, read forwards.
  double loadOnFingerHolds = 1.0;
  double loadOnGoodHolds = 0.35;

  // --- Recovery ---------------------------------------------------------
  // A month from redlined to clear, against skin's six nights. That gap is
  // the point: skin says what you can do this week and load says what you
  // can do this season, and a player who only listens to skin walks into
  // the second one without noticing.
  double recoveryPerNight = 3.2;
  // Resting properly is worth about twice sleeping it off, which is what
  // makes a rest day something you choose rather than something you lose.
  double restDayBonus = 3.0;

  // --- Getting hurt -----------------------------------------------------
  // Below the threshold nothing happens, ever. Above it the chance climbs
  // with how far past you are, rolled once on any day you climbed — so an
  // injury is always something you were warned about, never weather.
  double injuryThreshold = 62.0;
  double injuryChancePerPoint = 0.0055;

  // How bad, and how long. Severity 0..1 rolls low-biased: most injuries
  // are a fortnight of annoyance, and the season-ender is rare and real.
  double injuryDaysBase = 8.0;
  double injuryDaysPerSeverity = 44.0;

  // What it costs while it holds you, in grade units on the holds it bites.
  // Above dirtGradePenalty is wrong (a hurt climber is not a filthy route)
  // and below skinGradePenalty (1.4) would make it noise, so: between.
  double injuryGradePenalty = 2.6;
  double injuryBiteElsewhere = 0.25;

  // Climbing on it. This is the actual decision an injured climber makes,
  // and it has to be a real gamble both ways: you can get away with it.
  //
  // Aggravation raises *severity*, and the days left are recomputed from
  // that rather than added to — so a bad injury is bounded by what the
  // worst injury costs, not by how many times you were stupid. Adding days
  // per aggravation instead produced a 226-day injury out of one bad
  // fortnight, which is not a season-ender, it is a bug wearing one's coat.
  double aggravateChance = 0.30;
  double aggravateSeverity = 0.18;

  // An injury leaves you wary long after it stops hurting.
  double injuryPsycheCost = 0.15;

  // --- Physio -----------------------------------------------------------
  // The one thing in the game money buys that gives you climbing back
  // rather than moving it around: a session takes days off the clock.
  // Priced against a week of bills ($85), because that is the trade — you
  // are buying a fortnight of your season back, and it hurts.
  double physioCost = 95.0;
  int physioDaysSaved = 6;
  int physioDaysBetween = 7;   // no buying your way out in an afternoon
};

const char* InjuryName(InjuryKind kind);

// How long an injury of this severity holds you. The single source of that
// mapping — a fresh injury and an aggravated one must agree, or the ceiling
// is not a ceiling.
int InjuryDaysFor(double severity, const BodyDials& dials = BodyDials{});

// Does this injury bite on this kind of hold? A pulley is over on crimps
// and fine on slopers; a shoulder is the reverse. Everything bites a little
// everywhere, because nothing that hurts ever fully leaves you alone.
double InjuryBiteOn(InjuryKind kind, HoldType hold,
                    const BodyDials& dials = BodyDials{});

// What a burn on this route did to you. `challenge` is 0..1, the trainer's
// own measure of how hard the route asked you to try.
void AccrueLoad(Climber& climber, double challenge, HoldType hardestHold,
                const BodyDials& dials = BodyDials{});

// Load from something that is not a burn — an hour on the board, and
// whatever training gets added later. One clamp, in one place.
void AddLoad(Climber& climber, double amount,
             const BodyDials& dials = BodyDials{});

// Overnight: load comes down, an injury counts down, and being hurt keeps
// its own slow grip on psyche. `restedToday` is the honest kind of rest —
// a day you did not pull on at all.
// `age` is passed rather than stored so a save can never disagree with the
// birthday. Recovery slows past 30 and the injury threshold comes down with
// it — which is the whole of what aging does mechanically, and it is a
// shrinking training budget rather than a smaller climber.
void BodyDay(Climber& climber, bool restedToday, double age,
             const BodyDials& dials = BodyDials{},
             const AgeDials& ageDials = AgeDials{});

// Roll once for the day, on the body's own named stream so that getting
// hurt can never shift worldgen or how an attempt resolved. Returns true if
// today is the day. Only ever fires above the threshold.
bool RollForInjury(Climber& climber, const Rng& worldRng, int day,
                   const BodyDials& dials = BodyDials{},
                   const AgeDials& ageDials = AgeDials{});

// Pulling on while hurt. Returns true if you made it worse, which is the
// gamble — most of the time you get away with it, and that is what makes
// the choice a real one rather than a warning label.
bool ClimbOnIt(Climber& climber, const Rng& worldRng, int day, int attempt,
               const BodyDials& dials = BodyDials{});

// Money for time. False if you cannot afford it or it is not due yet.
bool Physio(Climber& climber, double& cash, int& lastPhysioDay, int today,
            const BodyDials& dials = BodyDials{});

bool IsHurt(const Climber& climber);

// "a pulley in the ring finger — three weeks, if you are sensible"
std::string InjuryText(const Climber& climber);

// "fresh", "carrying a load", "you are running on fumes"
std::string LoadText(const Climber& climber, const BodyDials& dials = BodyDials{});

}  // namespace dirtbag

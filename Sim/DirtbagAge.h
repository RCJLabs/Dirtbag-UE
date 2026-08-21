// Age, and the way it bends the curve.
//
// "Injuries and age bend the curve" (concepts/DIRTBAG.md). The obvious way
// to build this is to subtract numbers from an old climber, and it is the
// wrong way: it makes aging a punishment the player watches happen. What
// actually ends climbing careers is not being weaker. It is not being able
// to train as much — the session you could do twice a week at 25 you can do
// once at 45, and the one that used to cost three days now costs five.
//
// So age acts mostly through the budget that already exists. Training load
// (DirtbagBody.h) clears more slowly, the injury threshold comes down, and
// what you have keeps only as long as you keep asking for it. A climber who
// keeps training holds on a long time; one who stops slides. That is both
// the true shape and the more interesting one, because it leaves the player
// something to do about it.
//
// And not everything declines. Power goes first, fingers hold on much
// longer, and technique and head do not go at all — which is why the best
// climber at the crag is often the one nobody would pick in an arm
// wrestle. That asymmetry is the whole reason to have this system: it
// changes what kind of climber you are, rather than how good.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct AgeDials {
  // You turn up at the Lot at 24 with a van and no plan.
  double startAge = 24.0;
  // Mirrors ConditionsDials. If these two ever disagree the game runs a
  // birthday and a solstice on different calendars, and the only symptom
  // would be seasons slowly sliding against ages over a long career.
  int daysPerYear = 365;

  // --- Where each skill turns ------------------------------------------
  // Peaks from the sport's own shape: raw power goes first and goes
  // hardest; finger strength holds well into the thirties; endurance later
  // still. Technique and head are set past any plausible career because
  // they genuinely do not decline — a fifty-year-old reads a sequence
  // better than they ever have, and that is the thing that keeps them
  // climbing hard long after the campus board has stopped loving them.
  double powerPeak = 28.0;
  double fingersPeak = 34.0;
  double endurancePeak = 38.0;
  // There is deliberately no techniquePeak or headPeak. Two used to sit
  // here at a sentinel 999.0, read by nothing — AgeDay simply does not
  // touch those two skills, and says so where it does not. A dial nobody
  // reads is worse than no dial: it advertises a tuning that does not
  // exist, and somebody would eventually move one and wonder why nothing
  // happened. If head should ever decline, that is a design change with a
  // measurement behind it, not a number to twiddle.

  // Two different true things, and both are needed.
  //
  // **Decay** is what happens when you stop asking. A season of climbing
  // gains far more than this, so it is something you outrun by turning up
  // and it only catches the player who stops. At 1.6/year, a decade off the
  // couch past peak costs sixteen points of power — a grade and a half,
  // which is about what a decade off actually costs.
  double powerDeclinePerYear = 1.6;
  double fingersDeclinePerYear = 1.1;
  double enduranceDeclinePerYear = 0.9;

  // **The ceiling** is what your body will still hold, however hard you
  // train. Without it a climber who keeps turning up never declines at all
  // — measured, they pinned at 100 power from 28 to 52, because training
  // gains at the ceiling (about 8 points a year) outrun decay (1.6) five to
  // one. Nobody climbs at 52 the way they did at 28, and no amount of
  // turning up changes that. Decay is what you can do something about; this
  // is what you cannot.
  double ceilingLostPerYear = 2.2;
  double ceilingFloor = 35.0;

  // --- Recovery ---------------------------------------------------------
  // The real mechanic. Load clears more slowly every year past this, so an
  // older climber's training budget shrinks even though their numbers have
  // not moved. This is what makes a fortnight's plan at 45 different from
  // the same plan at 25.
  double recoveryHoldsUntil = 30.0;
  double recoveryLostPerYear = 0.018;   // fraction, compounding on the dial
  double recoveryFloor = 0.45;          // never worse than this

  // And you get hurt more easily, which is the same story from the other
  // side: the threshold comes down as the tissue stops forgiving.
  double injuryThresholdLostPerYear = 0.7;
  double injuryThresholdFloor = 34.0;
};

// How old you are, in years, on this day.
double AgeOn(int day, const AgeDials& dials = AgeDials{});

// What your body will still hold in a skill at this age, however hard you
// train. Full until the skill's own peak, then falling. Technique and head
// have no peak inside a career and so no ceiling either.
double MaxSkillFor(double age, double peak, double lostPerYear,
                   const AgeDials& dials = AgeDials{});

// A year's worth of not asking anything of a skill. Applied nightly at
// 1/daysPerYear, so it is invisible day to day and unmistakable across a
// career — which is exactly how it feels.
void AgeDay(Climber& climber, int day, const AgeDials& dials = AgeDials{});

// What age does to the body's two dials. Both are multipliers/offsets the
// body layer applies rather than numbers stored anywhere, so a save can
// never disagree with the birthday.
double RecoveryFactorFor(double age, const AgeDials& dials = AgeDials{});
double InjuryThresholdFor(double age, double baseThreshold,
                          const AgeDials& dials = AgeDials{});

// "31 — still going up" / "44 — the good years, if you are careful"
std::string AgeText(double age, const AgeDials& dials = AgeDials{});

}  // namespace dirtbag

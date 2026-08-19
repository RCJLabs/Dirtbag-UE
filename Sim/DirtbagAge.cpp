#include "DirtbagAge.h"

#include <algorithm>

namespace dirtbag {

namespace {

// One skill's nightly slide, given where its peak is. Nothing happens
// before the peak — a 24-year-old is not losing anything, and a system that
// quietly taxed them from day one would be a bug nobody could see.
double DeclineToday(double age, double peak, double perYear,
                    const AgeDials& dials) {
  if (age <= peak) return 0.0;
  return perYear / static_cast<double>(std::max(1, dials.daysPerYear));
}

void Slide(double& skill, double amount) {
  // Never below zero, and never below what a beginner walks in with: the
  // point is a career bending, not a person unlearning how to climb.
  skill = std::max(0.0, skill - amount);
}

}  // namespace

double MaxSkillFor(double age, double peak, double lostPerYear,
                   const AgeDials& dials) {
  if (age <= peak) return 100.0;
  return std::max(dials.ceilingFloor, 100.0 - lostPerYear * (age - peak));
}

double AgeOn(int day, const AgeDials& dials) {
  const int period = std::max(1, dials.daysPerYear);
  // Day 1 is the day you arrive, so it is the birthday rather than a day
  // past it.
  return dials.startAge +
         static_cast<double>(day - 1) / static_cast<double>(period);
}

void AgeDay(Climber& climber, int day, const AgeDials& dials) {
  const double age = AgeOn(day, dials);
  Skills& s = climber.skills;
  Slide(s.power,
        DeclineToday(age, dials.powerPeak, dials.powerDeclinePerYear, dials));
  Slide(s.fingers, DeclineToday(age, dials.fingersPeak,
                                dials.fingersDeclinePerYear, dials));
  Slide(s.endurance, DeclineToday(age, dials.endurancePeak,
                                  dials.enduranceDeclinePerYear, dials));

  // And the ceiling, which is the part training cannot argue with. It only
  // bites a climber who is actually at it — somebody at 60 power aged 45 is
  // untouched, because age was never what was limiting them.
  s.power = std::min(s.power, MaxSkillFor(age, dials.powerPeak,
                                          dials.ceilingLostPerYear, dials));
  s.fingers = std::min(s.fingers, MaxSkillFor(age, dials.fingersPeak,
                                              dials.ceilingLostPerYear, dials));
  s.endurance =
      std::min(s.endurance, MaxSkillFor(age, dials.endurancePeak,
                                        dials.ceilingLostPerYear, dials));
  // Technique and head are deliberately absent. A fifty-year-old reads a
  // sequence better than they ever have, and that is the thing that keeps
  // them climbing hard long after the campus board has stopped loving them.
}

double RecoveryFactorFor(double age, const AgeDials& dials) {
  if (age <= dials.recoveryHoldsUntil) return 1.0;
  const double years = age - dials.recoveryHoldsUntil;
  return std::max(dials.recoveryFloor,
                  1.0 - dials.recoveryLostPerYear * years);
}

double InjuryThresholdFor(double age, double baseThreshold,
                          const AgeDials& dials) {
  if (age <= dials.recoveryHoldsUntil) return baseThreshold;
  const double years = age - dials.recoveryHoldsUntil;
  return std::max(dials.injuryThresholdFloor,
                  baseThreshold - dials.injuryThresholdLostPerYear * years);
}

std::string AgeText(double age, const AgeDials& dials) {
  const int years = static_cast<int>(age);
  std::string out = std::to_string(years);
  out += " — ";
  if (age < dials.powerPeak) {
    out += "still going up";
  } else if (age < dials.fingersPeak) {
    out += "the strong years";
  } else if (age < dials.endurancePeak) {
    out += "the good years, if you are careful";
  } else if (age < 50.0) {
    out += "you climb smarter than you pull now";
  } else {
    out += "still here, which is most of it";
  }
  return out;
}

}  // namespace dirtbag

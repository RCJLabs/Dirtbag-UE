#include "DirtbagAilments.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

// --- sickness -----------------------------------------------------------

bool SicknessDay(Sickness& sick, const Climber& climber, double hunger,
                 double warmth, const Rng& worldRng, int day,
                 const AilmentDials& d) {
  if (sick.active) {
    sick.daysLeft--;
    if (sick.daysLeft <= 0) {
      sick.active = false;
      sick.daysLeft = 0;
      sick.severity = 0.0;
      sick.medicated = false;
    }
    return false;
  }

  // **Only ever off how you are living.** A fed, rested climber in a warm
  // van essentially never gets ill; a hungry one sleeping cold on a
  // wrecked body does. That is not a dice roll about them, it is a
  // description of them -- which is why sickness is allowed to be the one
  // thing in this file that is not your fault and still not weather.
  Rng rng = worldRng.Derive("sick#" + std::to_string(day));
  const double hungerAt = Clamp01(hunger / 100.0);
  const double loadAt = Clamp01(climber.load / 100.0);
  const double coldAt = 1.0 - Clamp01(warmth);
  const double chance =
      d.sickBaseChance * (1.0 + (d.sickHungerFactor - 1.0) * hungerAt) *
      (1.0 + (d.sickLoadFactor - 1.0) * loadAt) *
      (1.0 + (d.sickColdFactor - 1.0) * coldAt);
  if (rng.NextDouble() >= chance) return false;

  sick.active = true;
  sick.caught++;
  // Low-biased, like an injury: most of them are a bad week.
  const double roll = rng.NextDouble();
  sick.severity = roll * roll;
  sick.daysLeft = std::max(
      1, static_cast<int>(std::lround(d.sickDaysBase +
                                      d.sickDaysPerSeverity * sick.severity)));
  sick.medicated = false;
  return true;
}

bool TakeSomethingForIt(Sickness& sick, double& cash, const AilmentDials& d) {
  if (!sick.active || sick.medicated) return false;
  if (cash < d.medsCost) return false;
  cash -= d.medsCost;
  sick.medicated = true;
  // A chunk off what is left, not off what it was -- eleven dollars on day
  // six of a seven-day cold buys you most of a day, which is correct.
  const int off = static_cast<int>(
      std::lround(static_cast<double>(sick.daysLeft) * d.medsDaysOff));
  sick.daysLeft = std::max(1, sick.daysLeft - off);
  return true;
}

double SickPenalty(const Sickness& sick, const AilmentDials& d) {
  if (!sick.active) return 0.0;
  // Flat across every hold, because you are ill rather than injured --
  // there is no such thing as a cold that is fine on slopers.
  return d.sickGradePenalty * (0.45 + 0.55 * sick.severity);
}

std::string SickText(const Sickness& sick, const AilmentDials& d) {
  if (!sick.active) return std::string();
  const std::string how = sick.severity > 0.6   ? "properly ill"
                          : sick.severity > 0.3 ? "rough"
                                                : "under the weather";
  std::string out = "You are " + how + ".";
  if (sick.medicated) {
    out += "  You took something.";
  } else {
    out += "  $" + std::to_string(static_cast<int>(d.medsCost)) +
           " would take the edge off it.";
  }
  return out;
}

// --- teeth --------------------------------------------------------------

const char* ToothName(ToothStage s) {
  switch (s) {
    case ToothStage::Twinge: return "a twinge";
    case ToothStage::Ache: return "an ache";
    case ToothStage::Abscess: return "an abscess";
    case ToothStage::Fine:
    default: return "fine";
  }
}

namespace {

int StageLasts(ToothStage s, const AilmentDials& d) {
  switch (s) {
    case ToothStage::Twinge: return d.toothTwingeAfter;
    case ToothStage::Ache: return d.toothAcheAfter;
    case ToothStage::Abscess: return d.toothAbscessAfter;
    case ToothStage::Fine:
    default: return 0;
  }
}

}  // namespace

bool TeethDay(Teeth& teeth, const Rng& worldRng, int day,
              const AilmentDials& d) {
  Rng rng = worldRng.Derive("teeth#" + std::to_string(day));

  if (teeth.stage == ToothStage::Fine) {
    // A slow certainty rather than an event. The roll is the rate at which
    // it restarts, not a die you might dodge forever.
    if (rng.NextDouble() >= d.toothStartChance) return false;
    teeth.stage = ToothStage::Twinge;
    teeth.sinceDay = day;
    teeth.worstEver = std::max(teeth.worstEver, 1);
    return true;
  }

  // **It only ever goes one way.** Nothing here improves with rest, and
  // that is the whole design: it is the game's one clock that money is the
  // only answer to.
  if (day - teeth.sinceDay < StageLasts(teeth.stage, d)) return false;

  if (teeth.stage == ToothStage::Abscess) {
    // Except at the far end, where it does resolve itself -- the worst
    // possible way. The tooth comes out and it stops hurting, which is
    // what bounds never paying to a year of misery and a tooth rather
    // than the rest of a career.
    teeth.stage = ToothStage::Fine;
    teeth.sinceDay = day;
    teeth.lost++;
    return true;
  }

  teeth.stage = teeth.stage == ToothStage::Twinge ? ToothStage::Ache
                                                  : ToothStage::Abscess;
  teeth.sinceDay = day;
  teeth.worstEver =
      std::max(teeth.worstEver, static_cast<int>(teeth.stage));
  return true;
}

double ToothPrice(ToothStage stage, const AilmentDials& d) {
  switch (stage) {
    case ToothStage::Twinge: return d.toothTwingeCost;
    case ToothStage::Ache: return d.toothAcheCost;
    case ToothStage::Abscess: return d.toothAbscessCost;
    case ToothStage::Fine:
    default: return 0.0;
  }
}

bool FixTheTooth(Teeth& teeth, double& cash, int day,
                 const AilmentDials& d) {
  if (teeth.stage == ToothStage::Fine) return false;
  const double price = ToothPrice(teeth.stage, d);
  if (cash < price) return false;
  cash -= price;
  teeth.stage = ToothStage::Fine;
  teeth.sinceDay = day;
  teeth.fixes++;
  return true;
}

double ToothPsycheCost(const Teeth& teeth, const AilmentDials& d) {
  switch (teeth.stage) {
    case ToothStage::Ache: return d.toothAchePsyche;
    case ToothStage::Abscess: return d.toothAbscessPsyche;
    case ToothStage::Twinge:
    case ToothStage::Fine:
    default: return 0.0;   // a twinge is not a mood, it is a warning
  }
}

double ToothGradePenalty(const Teeth& teeth, const AilmentDials& d) {
  return teeth.stage == ToothStage::Abscess ? d.toothAbscessGrade : 0.0;
}

std::string TeethText(const Teeth& teeth, const AilmentDials& d) {
  switch (teeth.stage) {
    case ToothStage::Twinge:
      return "A tooth twinges on cold water.  $" +
             std::to_string(static_cast<int>(d.toothTwingeCost)) +
             " now, and it will not be that later.";
    case ToothStage::Ache:
      return "The tooth aches all the time now.  $" +
             std::to_string(static_cast<int>(d.toothAcheCost)) + ".";
    case ToothStage::Abscess:
      return "The tooth is an abscess. It is the main thing about your "
             "week.  $" +
             std::to_string(static_cast<int>(d.toothAbscessCost)) + ".";
    case ToothStage::Fine:
    default: return std::string();
  }
}

// --- prehab -------------------------------------------------------------

bool DoPrehab(Upkeep& upkeep, double& hour, int day,
              const AilmentDials& d) {
  if (upkeep.lastPrehabDay == day) return false;
  hour += d.prehabHours;
  // The streak survives a few missed mornings, because life happens and a
  // habit you lose by going to a wedding is not a habit, it is a chore.
  if (day - upkeep.lastPrehabDay <= d.prehabGraceDays + 1) {
    upkeep.prehabStreak++;
  } else {
    upkeep.prehabStreak = 1;
  }
  upkeep.lastPrehabDay = day;
  upkeep.prehabDays++;
  return true;
}

void UpkeepDay(Upkeep& upkeep, int day, const AilmentDials& d) {
  if (upkeep.prehabStreak <= 0) return;
  if (day - upkeep.lastPrehabDay > d.prehabGraceDays) {
    upkeep.prehabStreak = 0;
  }
}

double PrehabRisk(const Upkeep& upkeep, const AilmentDials& d) {
  if (upkeep.prehabStreak <= 0 || d.prehabStreakFor <= 0) return 1.0;
  const double at =
      Clamp01(static_cast<double>(upkeep.prehabStreak) /
              static_cast<double>(d.prehabStreakFor));
  // **Lowers the odds and never removes them.** A climber who has done
  // their twenty minutes every morning for a year still pops a pulley.
  return 1.0 - d.prehabRiskCut * at;
}

// --- the shrink ---------------------------------------------------------

bool SeeTheShrink(Upkeep& upkeep, Climber& climber, double& cash, int day,
                  const AilmentDials& d) {
  if (cash < d.shrinkCost) return false;
  if (upkeep.lastShrinkDay > 0 &&
      day - upkeep.lastShrinkDay < d.shrinkDaysBetween) {
    return false;
  }
  cash -= d.shrinkCost;
  upkeep.lastShrinkDay = day;
  upkeep.shrinkSessions++;
  climber.psyche = Clamp01(climber.psyche + d.shrinkPsyche);
  return true;
}

double PsycheFloor(const Upkeep& upkeep, double baseline, int day,
                   const AilmentDials& d) {
  if (upkeep.lastShrinkDay <= 0) return baseline;
  if (day - upkeep.lastShrinkDay > d.shrinkLasts) return baseline;
  // What a rest day cannot do: move where you drift back to.
  return Clamp01(baseline + d.shrinkFloorLift);
}

std::string UpkeepText(const Upkeep& upkeep, int day,
                       const AilmentDials& d) {
  std::string out;
  if (upkeep.prehabStreak > 0) {
    const double risk = PrehabRisk(upkeep, d);
    out = std::to_string(upkeep.prehabStreak) +
          (upkeep.prehabStreak == 1 ? " morning of it." : " mornings of it.");
    if (risk < 0.99) {
      out += "  " +
             std::to_string(static_cast<int>(std::lround(
                 (1.0 - risk) * 100.0))) +
             "% off the odds of something going.";
    }
  }
  if (upkeep.lastShrinkDay > 0 &&
      day - upkeep.lastShrinkDay <= d.shrinkLasts) {
    if (!out.empty()) out += "  ";
    out += "Things have been feeling more manageable.";
  }
  return out;
}

}  // namespace dirtbag

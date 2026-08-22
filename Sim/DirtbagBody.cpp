#include "DirtbagBody.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

const char* InjuryName(InjuryKind kind) {
  switch (kind) {
    case InjuryKind::Pulley:    return "a pulley in the ring finger";
    case InjuryKind::Lumbrical: return "a lumbrical tear";
    case InjuryKind::Elbow:     return "climber's elbow";
    case InjuryKind::Shoulder:  return "a shoulder that clicks";
  }
  return "something that is not right";
}

int InjuryDaysFor(double severity, const BodyDials& dials) {
  return static_cast<int>(dials.injuryDaysBase +
                          dials.injuryDaysPerSeverity * Clamp01(severity));
}

double InjuryBiteOn(InjuryKind kind, HoldType hold, const BodyDials& dials) {
  const double elsewhere = dials.injuryBiteElsewhere;
  switch (kind) {
    case InjuryKind::Pulley:
      // Crimps are over. Pockets nearly so. Everything else is fine, which
      // is why a hurt climber becomes a sloper climber for a month.
      if (hold == HoldType::Crimp) return 1.0;
      if (hold == HoldType::Pocket) return 0.8;
      if (hold == HoldType::Pinch) return 0.5;
      return elsewhere;
    case InjuryKind::Lumbrical:
      // One hold type, absolutely, and it takes forever.
      if (hold == HoldType::Pocket) return 1.0;
      return elsewhere * 0.5;
    case InjuryKind::Elbow:
      // The democratic one. Nothing is impossible and nothing is free,
      // which is exactly why nobody ever rests it properly.
      return 0.55;
    case InjuryKind::Shoulder:
      // Anything you have to pull sideways or catch. Crimping is fine, and
      // that is the trap: it feels better than it is.
      if (hold == HoldType::Sloper) return 1.0;
      if (hold == HoldType::Dyno) return 1.0;
      if (hold == HoldType::Jug) return 0.5;
      return elsewhere;
  }
  return elsewhere;
}

void AddLoad(Climber& climber, double amount, const BodyDials& dials) {
  if (amount <= 0.0) return;
  climber.load = std::min(dials.loadCeiling, climber.load + amount);
}

void AccrueLoad(Climber& climber, double challenge, HoldType hardestHold,
                const BodyDials& dials) {
  const bool fingerHold =
      hardestHold == HoldType::Crimp || hardestHold == HoldType::Pocket;
  const double where =
      fingerHold ? dials.loadOnFingerHolds : dials.loadOnGoodHolds;
  AddLoad(climber, dials.loadPerAttempt * Clamp01(challenge) * where, dials);
}

void BodyDay(Climber& climber, bool restedToday, double age,
             const BodyDials& dials, const AgeDials& ageDials) {
  const double back =
      (dials.recoveryPerNight + (restedToday ? dials.restDayBonus : 0.0)) *
      RecoveryFactorFor(age, ageDials);
  climber.load = std::max(0.0, climber.load - back);

  if (climber.injury.active) {
    climber.injury.daysLeft--;
    if (climber.injury.daysLeft <= 0) {
      climber.injury.active = false;
      climber.injury.daysLeft = 0;
      climber.injury.severity = 0.0;
    }
  }
}

bool RollForInjury(Climber& climber, const Rng& worldRng, int day,
                   const BodyDials& dials, const AgeDials& ageDials) {
  // Already hurt is not hurt again; that is what ClimbOnIt is for.
  if (climber.injury.active) return false;
  const double threshold = InjuryThresholdFor(AgeOn(day, ageDials),
                                              dials.injuryThreshold, ageDials);
  const double over = climber.load - threshold;
  if (over <= 0.0) return false;   // below the line, never, no matter what

  // Its own stream. Getting hurt must not shift worldgen, the weather, or
  // how any attempt resolved — the golden vectors depend on it.
  Rng rng = worldRng.Derive("body#" + std::to_string(day));
  if (!rng.Chance(std::min(0.95, over * dials.injuryChancePerPoint))) {
    return false;
  }

  climber.injury.active = true;
  climber.injury.kind = static_cast<InjuryKind>(
      static_cast<int>(rng.NextDouble() * kInjuryKindCount) %
      kInjuryKindCount);
  // Low-biased: most injuries are a fortnight of annoyance and the
  // season-ender is rare. Squaring a uniform roll is the cheapest honest
  // way to say that, and it keeps the tail real rather than clipped.
  const double roll = rng.NextDouble();
  climber.injury.severity = Clamp01(roll * roll);
  climber.injury.daysLeft = InjuryDaysFor(climber.injury.severity, dials);
  climber.psyche = std::max(0.05, climber.psyche - dials.injuryPsycheCost);
  return true;
}

bool TweakSomething(Climber& climber, const Rng& worldRng, int day,
                    int attempt, double challenge, HoldType hardestHold,
                    double warmth, const BodyDials& dials,
                    const AgeDials& ageDials) {
  // Already hurt is ClimbOnIt's question, not this one's.
  if (climber.injury.active) return false;
  if (challenge < dials.tweakChallengeFloor) return false;

  // Ramp from the floor to full challenge, so "at your limit" is the
  // dangerous place and just-over-the-floor is barely a risk at all.
  const double ramp = (challenge - dials.tweakChallengeFloor) /
                      std::max(1e-9, 1.0 - dials.tweakChallengeFloor);

  const bool fingerHold =
      hardestHold == HoldType::Crimp || hardestHold == HoldType::Pocket;
  double chance = dials.tweakChanceAtLimit * Clamp01(ramp);
  if (!fingerHold) chance *= dials.tweakGoodHoldFactor;
  if (warmth < dials.tweakWarmEnough) chance *= dials.tweakColdFactor;
  // Tendons age first. Capped, so an old career is fragile rather than
  // cursed.
  const double age = AgeOn(day, ageDials);
  chance *= std::min(2.0, 1.0 + std::max(0.0, age - 30.0) *
                                    dials.tweakAgePerYear);

  // Its own stream, per burn -- twenty burns are twenty separate gambles,
  // and nothing here may shift how any attempt resolves.
  Rng rng = worldRng.Derive("tweak#" + std::to_string(day) + "#" +
                            std::to_string(attempt));
  if (!rng.Chance(chance)) return false;

  climber.injury.active = true;
  // What pops is what you were pulling on: fingers on the fingery holds,
  // shoulders on the big moves off the rest. Not uniform like the chronic
  // path, because a tweak has a location the way overtraining does not.
  if (fingerHold) {
    climber.injury.kind = hardestHold == HoldType::Pocket
                              ? InjuryKind::Lumbrical
                              : InjuryKind::Pulley;
  } else {
    climber.injury.kind =
        rng.NextDouble() < 0.65 ? InjuryKind::Shoulder : InjuryKind::Elbow;
  }
  // Same low-biased severity as the chronic path: most tweaks are a
  // fortnight of annoyance, and the season-ender is rare and real.
  const double roll = rng.NextDouble();
  climber.injury.severity = Clamp01(roll * roll);
  climber.injury.daysLeft = InjuryDaysFor(climber.injury.severity, dials);
  climber.psyche = std::max(0.05, climber.psyche - dials.injuryPsycheCost);
  return true;
}

bool ClimbOnIt(Climber& climber, const Rng& worldRng, int day, int attempt,
               const BodyDials& dials) {
  if (!climber.injury.active) return false;
  // Per attempt, not per day: the gamble is taken one burn at a time, and
  // a policy that pulls on twenty times should be twenty times as sorry.
  Rng rng = worldRng.Derive("aggravate#" + std::to_string(day) + "#" +
                            std::to_string(attempt));
  if (!rng.Chance(dials.aggravateChance)) return false;

  climber.injury.severity = Clamp01(climber.injury.severity +
                                    dials.aggravateSeverity);
  // Recomputed, never accumulated: however many times you pull on it, the
  // worst it can be is the worst an injury is.
  climber.injury.daysLeft =
      std::max(climber.injury.daysLeft, InjuryDaysFor(climber.injury.severity,
                                                      dials));
  return true;
}

bool Physio(Climber& climber, double& cash, int& lastPhysioDay, int today,
            const BodyDials& dials) {
  if (!climber.injury.active) return false;
  if (cash < dials.physioCost) return false;
  // No buying your way out of a season in an afternoon.
  if (lastPhysioDay > 0 && today - lastPhysioDay < dials.physioDaysBetween) {
    return false;
  }

  cash -= dials.physioCost;
  lastPhysioDay = today;
  climber.injury.daysLeft =
      std::max(1, climber.injury.daysLeft - dials.physioDaysSaved);
  return true;
}

bool IsHurt(const Climber& climber) { return climber.injury.active; }

std::string InjuryText(const Climber& climber) {
  if (!climber.injury.active) return "";
  std::string out = InjuryName(climber.injury.kind);
  const int weeks = (climber.injury.daysLeft + 6) / 7;
  out += " — ";
  if (weeks <= 1) {
    out += "another week";
  } else {
    out += std::to_string(weeks) + " weeks";
  }
  out += ", if you are sensible";
  return out;
}

std::string LoadText(const Climber& climber, const BodyDials& dials) {
  if (climber.load < 20.0) return "fresh";
  if (climber.load < 40.0) return "warmed into the season";
  if (climber.load < dials.injuryThreshold) return "carrying a load";
  if (climber.load < 85.0) return "everything aches; this is the warning";
  return "you are running on fumes and you know it";
}

int LoadWarning(const Climber& climber, const BodyDials& dials) {
  // The bands are LoadText's, deliberately: the colour and the sentence are
  // two readings of one fact and must never disagree.
  if (climber.load < 40.0) return 0;
  if (climber.load < dials.injuryThreshold) return 1;
  return 2;
}

}  // namespace dirtbag

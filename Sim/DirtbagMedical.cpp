#include "DirtbagMedical.h"

#include <algorithm>
#include <cmath>

#include "DirtbagBody.h"

namespace dirtbag {

const char* DiagnosisName(Diagnosis d) {
  switch (d) {
    case Diagnosis::Guessed: return "seen";
    case Diagnosis::Scanned: return "scanned";
    case Diagnosis::None:
    default: return "unknown";
  }
}

const char* TreatmentName(Treatment t) {
  switch (t) {
    case Treatment::Physio: return "physio";
    case Treatment::Cortisone: return "cortisone";
    case Treatment::Surgery: return "surgery";
    case Treatment::Rest:
    default: return "rest";
  }
}

const char* ComebackName(Comeback c) {
  switch (c) {
    case Comeback::Resting: return "resting it";
    case Comeback::Mobility: return "moving it";
    case Comeback::GradedReturn: return "on the way back";
    case Comeback::Clear:
    default: return "fine";
  }
}

namespace {

double ShareOf(Comeback stage, const MedicalDials& d) {
  switch (stage) {
    case Comeback::Resting: return d.restingShare;
    case Comeback::Mobility: return d.mobilityShare;
    case Comeback::GradedReturn: return d.returnShare;
    case Comeback::Clear:
    default: return 0.0;
  }
}

// How long this stage should last, from the injury's own day count. The
// **whole** count rather than what is left, so the three stages always add
// up to the injury however far through it you are.
int StageLengthFor(const Climber& climber, Comeback stage,
                   const MedicalDials& d) {
  const int whole = InjuryDaysFor(climber.injury.severity);
  const double days = static_cast<double>(whole) * ShareOf(stage, d);
  return std::max(1, static_cast<int>(std::lround(days)));
}

}  // namespace

// --- knowing -----------------------------------------------------------

double BillFor(const Medical& med, double list, int day,
               const MedicalDials& d) {
  if (!CoverIsLive(med, day, d)) return list;
  return list * (1.0 - d.covers);
}

bool Diagnose(Medical& med, const Climber& climber, double& cash,
              Diagnosis tier, const Rng& worldRng, int day,
              const MedicalDials& d) {
  if (!climber.injury.active) return false;
  if (tier == Diagnosis::None) return false;
  // Knowing more is the only direction. Paying a physio to feel a joint a
  // machine has already photographed is money for nothing.
  if (static_cast<int>(tier) <= static_cast<int>(med.diagnosis)) return false;

  const double list =
      tier == Diagnosis::Scanned ? d.scanCost : d.guessCost;
  const double bill = BillFor(med, list, day, d);
  if (cash < bill) return false;
  cash -= bill;
  if (CoverIsLive(med, day, d)) med.claimsPaid += list - bill;

  med.diagnosis = tier;
  med.diagnoses++;
  med.treatedThisTime = true;

  if (tier == Diagnosis::Scanned) {
    // A machine looked at it. There is a number now.
    med.toldSeverity = climber.injury.severity;
  } else {
    // **Good hands, not perfect ones.** The error is the whole fog: treat
    // a 0.8 you were told was a 0.55 and you will come back too early.
    Rng rng = worldRng.Derive("diagnose#" + std::to_string(day));
    const double err = (rng.NextDouble() * 2.0 - 1.0) * d.guessError;
    med.toldSeverity =
        std::max(0.0, std::min(1.0, climber.injury.severity + err));
  }
  return true;
}

// --- treating ----------------------------------------------------------

bool TakeTheShot(Medical& med, Climber& climber, double& cash, int day,
                 const MedicalDials& d) {
  if (!climber.injury.active) return false;
  if (med.stage == Comeback::Clear) return false;
  // **One of these per injury.** Without it a climber with money in the
  // bank takes the shot every morning of the same lay-off: measured, a
  // rich career had **294 surgeries out of one injury** and an impatient
  // one 316 cortisone shots. Neither is a decision; both are a loop.
  if (med.treatment == Treatment::Cortisone ||
      med.treatment == Treatment::Surgery) {
    return false;
  }
  const double bill = BillFor(med, d.cortisoneCost, day, d);
  if (cash < bill) return false;
  cash -= bill;
  if (CoverIsLive(med, day, d)) med.claimsPaid += d.cortisoneCost - bill;

  med.treatment = Treatment::Cortisone;
  med.treatedThisTime = true;
  med.shotsTaken++;
  const int j = static_cast<int>(climber.injury.kind);
  med.shots[j]++;
  // **Works now.** The acute stages are over; you are climbing this week.
  med.stage = Comeback::GradedReturn;
  med.stageStarted = day;
  med.stageDays = StageLengthFor(climber, Comeback::GradedReturn, d);
  // **Costs you forever.** Nothing takes this off but a knife.
  med.joints[j] = std::min(1.0, med.joints[j] + d.cortisoneJointCost);
  return true;
}

bool HaveSurgery(Medical& med, Climber& climber, double& cash, int day,
                 const MedicalDials& d) {
  if (!climber.injury.active) return false;
  // **Nobody operates on a guess.** The scan is not optional here, which
  // is what makes the expensive diagnosis worth having.
  if (med.diagnosis != Diagnosis::Scanned) return false;
  // And nobody has the same operation twice in a fortnight. See the note
  // in `TakeTheShot`.
  if (med.treatment == Treatment::Cortisone ||
      med.treatment == Treatment::Surgery) {
    return false;
  }
  if (climber.injury.severity < d.surgeryNeedsSeverity) return false;
  const double bill = BillFor(med, d.surgeryCost, day, d);
  if (cash < bill) return false;
  cash -= bill;
  if (CoverIsLive(med, day, d)) med.claimsPaid += d.surgeryCost - bill;

  med.treatment = Treatment::Surgery;
  med.treatedThisTime = true;
  med.surgeries++;
  const int j = static_cast<int>(climber.injury.kind);
  // The only thing in this game that takes damage *off* a joint.
  med.joints[j] *= (1.0 - d.surgeryJointRepair);
  // And most of a season on the other side of it.
  climber.injury.daysLeft =
      std::max(climber.injury.daysLeft,
               static_cast<int>(std::lround(d.surgeryDays)));
  med.stage = Comeback::Resting;
  med.stageStarted = day;
  med.stageDays = static_cast<int>(std::lround(d.surgeryDays * d.restingShare));
  return true;
}

// --- the comeback ------------------------------------------------------

MedicalNight FinishInjury(Medical& med, Climber& climber, int day,
                          const MedicalDials& d) {
  MedicalNight out;
  if (!climber.injury.active) return out;

  const double sev = climber.injury.severity;
  out.healed = true;
  out.untreated = !med.treatedThisTime;
  double weight = d.scarFromSeverity * sev;
  if (med.rushedComebacks > 0) weight += d.scarFromRushing * sev;
  if (out.untreated) {
    weight += d.untreatedScar;
    med.untreatedInjuries++;
  }
  if (med.treatment == Treatment::Surgery) {
    // The knife leaves its own mark, and it is a small one.
    weight = d.surgeryScar;
  }
  if (weight > 0.0) {
    out.scarred = true;
    out.scarWeight = weight;
    med.scars.push_back(Scar{climber.injury.kind, weight, day});
  }

  climber.injury.active = false;
  climber.injury.staged = false;
  climber.injury.daysLeft = 0;
  climber.injury.severity = 0.0;
  med.stage = Comeback::Clear;
  med.stageDays = 0;
  med.rushedComebacks = 0;
  med.diagnosis = Diagnosis::None;
  med.treatment = Treatment::Rest;
  med.toldSeverity = 0.0;
  out.news = out.untreated
                 ? "It stopped hurting. You never did find out what it was."
                 : "Cleared to climb.";
  return out;
}

int DaysLeftInComeback(const Medical& med, const Climber& climber, int day,
                       const MedicalDials& d) {
  if (!climber.injury.active || med.stage == Comeback::Clear) return 0;
  int left = DaysLeftInStage(med, day);
  // Plus whatever stages are still ahead of this one.
  for (int s = static_cast<int>(med.stage) + 1;
       s <= static_cast<int>(Comeback::GradedReturn); s++) {
    left += StageLengthFor(climber, static_cast<Comeback>(s), d);
  }
  return left;
}

void StartComeback(Medical& med, Climber& climber, int day,
                   const MedicalDials& d) {
  // The comeback takes the clock off `BodyDay`. See `Injury::staged`.
  climber.injury.staged = true;
  med.diagnosis = Diagnosis::None;
  med.treatment = Treatment::Rest;
  med.toldSeverity = 0.0;
  med.treatedThisTime = false;
  med.stage = Comeback::Resting;
  med.stageStarted = day;
  med.stageDays = StageLengthFor(climber, Comeback::Resting, d);
}

bool StageIsDone(const Medical& med, int day) {
  if (med.stage == Comeback::Clear) return true;
  return day - med.stageStarted >= med.stageDays;
}

int DaysLeftInStage(const Medical& med, int day) {
  if (med.stage == Comeback::Clear) return 0;
  return std::max(0, med.stageDays - (day - med.stageStarted));
}

bool NextStage(Medical& med, Climber& climber, const Rng& worldRng, int day,
               const MedicalDials& d) {
  if (med.stage == Comeback::Clear) return false;

  const int early = DaysLeftInStage(med, day);
  bool setBack = false;
  if (early > 0) {
    // **Rolled once, on how early you are.** A decision, not a slow leak --
    // and one you can get away with, which is what makes it a gamble
    // rather than a warning label.
    Rng rng = worldRng.Derive("comeback#" + std::to_string(day));
    const double chance =
        std::min(0.95, d.earlyChancePerDay * static_cast<double>(early));
    if (rng.NextDouble() < chance) {
      setBack = true;
      med.rushedComebacks++;
      climber.injury.severity =
          std::min(1.0, climber.injury.severity + d.earlySeverityCost);
      climber.injury.daysLeft = InjuryDaysFor(climber.injury.severity);
      // Back to the start of this stage, on the new severity.
      med.stageStarted = day;
      med.stageDays = StageLengthFor(climber, med.stage, d);
      return true;
    }
    // Got away with it -- but it still counts as rushed, and that is what
    // the scar reads at the end.
    med.rushedComebacks++;
  }

  if (med.stage == Comeback::GradedReturn) {
    // **Choosing to end it heals it**, exactly as waiting it out does.
    // The first version only healed on the waiting path, so a climber who
    // clicked through the last stage came out the other side still
    // flagged hurt -- and the night tick started the whole comeback over.
    FinishInjury(med, climber, day, d);
    return setBack;
  }
  med.stage = med.stage == Comeback::Resting ? Comeback::Mobility
                                             : Comeback::GradedReturn;
  med.stageStarted = day;
  med.stageDays = StageLengthFor(climber, med.stage, d);
  return setBack;
}

double StagePenalty(const Medical& med, const MedicalDials& d) {
  switch (med.stage) {
    case Comeback::Resting: return d.restingPenalty;
    case Comeback::Mobility: return d.mobilityPenalty;
    case Comeback::GradedReturn: return d.returnPenalty;
    case Comeback::Clear:
    default: return 0.0;
  }
}

MedicalNight MedicalDay(Medical& med, Climber& climber, const Rng& worldRng,
                        int day, const MedicalDials& d) {
  MedicalNight out;

  if (!climber.injury.active) {
    med.stage = Comeback::Clear;
    return out;
  }

  // **Hurt with no comeback running is a state the game has to survive.**
  // It happens three ways and all of them are real: a save written before
  // this system existed, a test that builds an injury by hand, and any
  // future code path that sets `injury.active` and forgets to start the
  // clock. Without this the first night heals it outright -- `Clear` reads
  // as "this stage is done", the switch falls through to the last case,
  // and a forty-day injury evaporates overnight. A test caught exactly
  // that.
  if (med.stage == Comeback::Clear) {
    StartComeback(med, climber, day, d);
    return out;
  }

  // **An injury nobody looked at takes longer.** Not a punishment -- the
  // untreated path is reached by being broke, and this is what being broke
  // in a body costs.
  if (!med.treatedThisTime && StageIsDone(med, day) &&
      med.stage != Comeback::Clear) {
    const int stretched = static_cast<int>(
        std::lround(static_cast<double>(med.stageDays) *
                    (d.untreatedDaysMult - 1.0)));
    if (stretched > 0 && day - med.stageStarted < med.stageDays + stretched) {
      return out;
    }
  }

  if (!StageIsDone(med, day)) return out;

  // The stage ran itself out. Advancing on time is never a gamble, so this
  // is the one path that cannot set you back.
  switch (med.stage) {
    case Comeback::Resting:
      med.stage = Comeback::Mobility;
      med.stageStarted = day;
      med.stageDays = StageLengthFor(climber, Comeback::Mobility, d);
      break;
    case Comeback::Mobility:
      med.stage = Comeback::GradedReturn;
      med.stageStarted = day;
      med.stageDays = StageLengthFor(climber, Comeback::GradedReturn, d);
      break;
    case Comeback::GradedReturn:
    default:
      out = FinishInjury(med, climber, day, d);
      break;
  }
  // Keep the old clock honest for everything that still reads it -- the
  // HUD, the sponsor's days-hurt count, the guidebook's opinion of you.
  climber.injury.daysLeft = DaysLeftInComeback(med, climber, day, d);
  (void)worldRng;
  return out;
}

// --- what it leaves ----------------------------------------------------

double ScarWeight(const Scar& scar, int today, const MedicalDials& d) {
  const double years =
      std::max(0.0, static_cast<double>(today - scar.fromDay)) / 365.0;
  const double faded = scar.weight * (1.0 - d.scarFadePerYear * years);
  // Never to nothing: the floor is a fraction of what it *was*, not of
  // what it is now, or it collapses toward zero one night at a time.
  return std::max(scar.weight * d.scarFloor, faded);
}

double JointWear(const Medical& med, InjuryKind kind, int today,
                 const MedicalDials& d) {
  double wear = med.joints[static_cast<int>(kind)];
  for (const Scar& s : med.scars) {
    if (s.kind == kind) wear += ScarWeight(s, today, d);
  }
  return std::min(1.0, wear);
}

double JointRisk(const Medical& med, InjuryKind kind, int today,
                 const MedicalDials& d) {
  // The same history `JointWear` reads, priced as a multiplier. One
  // formula and one cap -- see `riskPerWear`.
  return 1.0 + JointWear(med, kind, today, d) * d.riskPerWear;
}

double BodyRisk(const Medical& med, int today, const MedicalDials& d) {
  double worst = 1.0;
  for (int i = 0; i < kInjuryKindCount; i++) {
    worst = std::max(
        worst, JointRisk(med, static_cast<InjuryKind>(i), today, d));
  }
  return worst;
}

// --- insurance ---------------------------------------------------------

bool BuyInsurance(Medical& med, const Climber& climber, int day,
                  const MedicalDials& d) {
  if (med.insured) return false;
  // Everybody tries this.
  if (d.refuseWhenHurt && climber.injury.active) return false;
  med.insured = true;
  med.insuredOnDay = day;
  return true;
}

void CancelInsurance(Medical& med) {
  med.insured = false;
  med.insuredOnDay = 0;
}

bool CoverIsLive(const Medical& med, int day, const MedicalDials& d) {
  if (!med.insured) return false;
  // **A wait before it pays**, so buying it the week before a planned
  // surgery is not a strategy.
  return day - med.insuredOnDay >= d.waitingDays;
}

bool InsuranceDay(Medical& med, double& cash, double& owed, int day,
                  const MedicalDials& d) {
  if (!med.insured) return false;
  if (d.premiumEveryDays <= 0) return false;
  if ((day - med.insuredOnDay) % d.premiumEveryDays != 0) return false;
  if (day == med.insuredOnDay) return false;
  // A bill like any other: it lands whether or not the money is there.
  const double take = std::min(cash, d.premium);
  cash -= take;
  owed += d.premium - take;
  med.premiumsPaid += d.premium;
  return true;
}

// --- what it reads like ------------------------------------------------

std::string MedicalText(const Medical& med, const Climber& climber, int day,
                        const MedicalDials& d) {
  if (!climber.injury.active) return std::string();
  const std::string what = InjuryName(climber.injury.kind);

  if (med.diagnosis == Diagnosis::None) {
    // **Deliberately vague.** This is the mechanic, not a missing string:
    // a climber with a sore finger does not know whether it is three weeks
    // or three months, and buying the number is the first decision.
    return "Something in the " + what +
           ". You do not know how bad, and it is " +
           ComebackName(med.stage) + ".";
  }

  const int left = DaysLeftInStage(med, day);
  const std::string how =
      med.toldSeverity > 0.66   ? "bad"
      : med.toldSeverity > 0.33 ? "not nothing"
                                : "minor";
  std::string out = "A " + how + " " + what + ", " +
                    ComebackName(med.stage) + ".";
  if (med.diagnosis == Diagnosis::Scanned) {
    out += left > 0 ? "  " + std::to_string(left) +
                          (left == 1 ? " day of this stage left."
                                     : " days of this stage left.")
                    : "  This stage is done.";
  } else {
    // A guess gets a feeling, not a countdown.
    out += left > 0 ? "  Not ready yet." : "  About ready, they reckon.";
  }
  (void)d;
  return out;
}

std::string HistoryText(const Medical& med, int today,
                        const MedicalDials& d) {
  std::string out;
  for (int i = 0; i < kInjuryKindCount; i++) {
    const InjuryKind k = static_cast<InjuryKind>(i);
    const double risk = JointRisk(med, k, today, d);
    if (risk <= 1.0001) continue;
    if (!out.empty()) out += "  ";
    out += std::string(InjuryName(k)) + ": ";
    if (med.shots[i] > 0) {
      out += std::to_string(med.shots[i]) +
             (med.shots[i] == 1 ? " shot" : " shots");
    } else {
      out += "old";
    }
    out += risk > 1.6 ? ", and it knows it." : ".";
  }
  return out;
}

}  // namespace dirtbag

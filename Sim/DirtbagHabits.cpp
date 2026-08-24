#include "DirtbagHabits.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

namespace {

double At(const Logbook& r, Did what) {
  return r.count[static_cast<int>(what)];
}

// A ratio that refuses to answer when the denominator is too thin. Without
// this, one burn on one line is a Grinder at 100%.
double Share(double part, double whole, double floor_) {
  if (whole < floor_ || whole <= 0.0) return 0.0;
  return part / whole;
}

}  // namespace

void RememberTo(Logbook& record, int day, const HabitDials& dials) {
  const int elapsed = day - record.asOfDay;
  if (elapsed <= 0) {
    // Same day, or a clock that went backwards. Neither is a reason to
    // decay anything, and a negative elapsed would *grow* the record.
    record.asOfDay = std::max(record.asOfDay, day);
    return;
  }
  const double half = std::max(1.0, dials.remembersDays);
  const double keep = std::pow(0.5, static_cast<double>(elapsed) / half);
  for (int i = 0; i < kDidCount; i++) record.count[i] *= keep;
  record.asOfDay = day;
}

void Note(Logbook& record, Did what, double amount, int day,
          const HabitDials& dials) {
  if (amount <= 0.0) return;
  RememberTo(record, day, dials);
  record.count[static_cast<int>(what)] += amount;
  if (what == Did::Burn) record.lifetimeBurns += amount;
  if (what == Did::DayOut) record.lifetimeDays += amount;
}

const char* HabitName(Habit h) {
  switch (h) {
    case Habit::None:         return "nothing in particular";
    case Habit::DawnPatrol:   return "dawn patrol";
    case Habit::Grinder:      return "grinding";
    case Habit::Tourist:      return "ticking";
    case Habit::NeverWarmsUp: return "never warming up";
    case Habit::GymRat:       return "indoors";
    case Habit::SkinOfSteel:  return "climbing on nothing";
    case Habit::KnowsWhenToStop: return "knowing when to stop";
    case Habit::RunsItOut:    return "running it out";
    case Habit::SewsItUp:     return "sewing it up";
    case Habit::kHabitCount:  break;
  }
  return "nothing in particular";
}

const char* HabitLine(Habit h) {
  switch (h) {
    case Habit::None:
      return "You climb the way most people climb.";
    case Habit::DawnPatrol:
      return "You are on it before the sun is. Most days.";
    case Habit::Grinder:
      return "One line. Everything else in the valley can wait.";
    case Habit::Tourist:
      return "A burn on everything and a second burn on nothing.";
    case Habit::NeverWarmsUp:
      return "Boots on, straight onto the hard one. It has not caught up "
             "with you yet.";
    case Habit::GymRat:
      return "Most of your days happen under a roof.";
    case Habit::SkinOfSteel:
      return "You pull on with nothing left on your tips and it does not "
             "seem to bother you.";
    case Habit::KnowsWhenToStop:
      return "You go home with something left in the tank. Most people "
             "cannot.";
    case Habit::RunsItOut:
      return "You climb a long way above the last piece, and you keep "
             "choosing to.";
    case Habit::SewsItUp:
      return "Nothing goes past you unprotected. It costs you, and you "
             "have made your peace with that.";
    case Habit::kHabitCount:
      break;
  }
  return "";
}

bool Doing(const Logbook& record, Habit h, int today, const HabitDials& dials) {
  Logbook now = record;
  RememberTo(now, today, dials);

  // The floors, and they are the whole reason a first afternoon does not
  // decide who you are.
  const double burns = At(now, Did::Burn);
  const double days = At(now, Did::DayOut);
  const double gearMoves = At(now, Did::MoveOnGear);

  switch (h) {
    case Habit::DawnPatrol:
      return Share(At(now, Did::DawnStart), days, dials.enoughDays) >=
             dials.dawnPatrolAt;
    case Habit::Grinder:
      return Share(At(now, Did::BurnOnOneLine), burns, dials.enoughBurns) >=
             dials.grinderAt;
    case Habit::Tourist:
      return Share(At(now, Did::LineTouched), burns, dials.enoughBurns) >=
             dials.touristAt;
    case Habit::NeverWarmsUp:
      return Share(At(now, Did::BurnAtYourLimit), burns, dials.enoughBurns) >=
             dials.neverWarmsUpAt;
    case Habit::GymRat:
      return Share(At(now, Did::DayIndoors), days, dials.enoughDays) >=
             dials.gymRatAt;
    case Habit::SkinOfSteel:
      return Share(At(now, Did::BurnOnThinSkin), burns, dials.enoughBurns) >=
             dials.skinOfSteelAt;
    case Habit::KnowsWhenToStop:
      return Share(At(now, Did::StoppedEarly), days, dials.enoughDays) >=
             dials.knowsWhenToStopAt;
    case Habit::RunsItOut:
      // Its own denominator: a boulderer is not running it out, they are
      // bouldering, and dividing by every burn would make a season of
      // plastic look like nerve.
      return Share(At(now, Did::MoveRunOut), gearMoves, dials.enoughBurns) >=
             dials.runsItOutAt;
    case Habit::SewsItUp:
      return Share(At(now, Did::PiecePlaced), gearMoves, dials.enoughBurns) >=
             dials.sewsItUpAt;
    case Habit::None:
    case Habit::kHabitCount:
      break;
  }
  return false;
}

std::vector<Habit> HabitsNow(const Logbook& record, int today,
                             const HabitDials& dials) {
  std::vector<Habit> out;
  for (int i = 1; i < kHabitCount; i++) {
    const Habit h = static_cast<Habit>(i);
    if (Doing(record, h, today, dials)) out.push_back(h);
  }
  return out;
}

const char* QuirkName(Quirk q) {
  switch (q) {
    case Quirk::None:            return "nobody in particular";
    case Quirk::MorningPerson:   return "a morning person";
    case Quirk::Obsessive:       return "obsessive";
    case Quirk::Magpie:          return "a magpie";
    case Quirk::Impatient:       return "impatient";
    case Quirk::PlasticMerchant: return "a plastic merchant";
    case Quirk::Leathery:        return "leathery";
    case Quirk::Cautious:        return "cautious";
    case Quirk::Bold:            return "bold";
    case Quirk::Fastidious:      return "fastidious";
    case Quirk::LightSleeper:    return "a light sleeper";
    case Quirk::BadWithMoney:    return "bad with money";
    case Quirk::Superstitious:   return "superstitious";
    case Quirk::Stubborn:        return "stubborn";
    case Quirk::Gregarious:      return "gregarious";
    case Quirk::Quiet:           return "quiet";
    case Quirk::kQuirkCount:     break;
  }
  return "nobody in particular";
}

const char* QuirkLine(Quirk q) {
  switch (q) {
    case Quirk::None:
      return "";
    case Quirk::MorningPerson:
      return "You wake up before the alarm now, and you have stopped "
             "pretending it is a discipline.";
    case Quirk::Obsessive:
      return "You do not really see a crag any more. You see the line and "
             "the rest of the rock it is on.";
    case Quirk::Magpie:
      return "You have been up more things than anybody in the valley and "
             "you could not tell them the moves on any of them.";
    case Quirk::Impatient:
      return "Warming up is for people with more time than you have.";
    case Quirk::PlasticMerchant:
      return "You know every set in the building and half the rock in the "
             "valley by sight only.";
    case Quirk::Leathery:
      return "Your fingers stopped being a thing you think about some years "
             "ago.";
    case Quirk::Cautious:
      return "You have been going home early for years and you are still "
             "here, which most of the people you started with are not.";
    case Quirk::Bold:
      return "The gear is a formality. It has been for a while.";
    case Quirk::Fastidious:
      return "Nothing goes past you unprotected, and nothing has come "
             "close to hurting you in years.";
    case Quirk::LightSleeper:
      return "You are awake at five whether or not there is anything to do "
             "about it.";
    case Quirk::BadWithMoney:
      return "It goes as fast as it arrives. It always has.";
    case Quirk::Superstitious:
      return "You do the same three things before you pull on and you are "
             "not going to be talked out of any of them.";
    case Quirk::Stubborn:
      return "You have never once been the person who suggested going home.";
    case Quirk::Gregarious:
      return "There is nobody at the Lot you have not sat down with.";
    case Quirk::Quiet:
      return "People find out what you have climbed by accident, usually "
             "years later.";
    case Quirk::kQuirkCount:
      break;
  }
  return "";
}

Quirk HardensInto(Habit h) {
  switch (h) {
    case Habit::DawnPatrol:   return Quirk::MorningPerson;
    case Habit::Grinder:      return Quirk::Obsessive;
    case Habit::Tourist:      return Quirk::Magpie;
    case Habit::NeverWarmsUp: return Quirk::Impatient;
    case Habit::GymRat:       return Quirk::PlasticMerchant;
    case Habit::SkinOfSteel:  return Quirk::Leathery;
    case Habit::KnowsWhenToStop: return Quirk::Cautious;
    case Habit::RunsItOut:    return Quirk::Bold;
    case Habit::SewsItUp:     return Quirk::Fastidious;
    case Habit::None:
    case Habit::kHabitCount:
      break;
  }
  return Quirk::None;
}

Habit HabitBehind(Quirk q) {
  for (int i = 1; i < kHabitCount; i++) {
    const Habit h = static_cast<Habit>(i);
    if (HardensInto(h) == q) return h;
  }
  return Habit::None;
}

bool IsPicked(Quirk q) {
  return q >= Quirk::LightSleeper && q < Quirk::kQuirkCount;
}

bool Has(const Quirks& q, Quirk which) {
  if (which == Quirk::None) return false;
  return std::find(q.held.begin(), q.held.end(), which) != q.held.end();
}

Quirk HabitsDay(Quirks& quirks, const Logbook& record, int today,
                const HabitDials& dials) {
  Quirk landed = Quirk::None;
  for (int i = 1; i < kHabitCount; i++) {
    const Habit h = static_cast<Habit>(i);
    const Quirk q = HardensInto(h);
    double& held = quirks.heldFor[i];

    if (Has(quirks, q)) {
      // Already who you are. Nothing left to earn, and nothing to drain —
      // a quirk does not lapse, which is the whole rule.
      continue;
    }
    if (Doing(record, h, today, dials)) {
      held += 1.0;
      if (held >= dials.quirkAfterDays) {
        quirks.held.push_back(q);
        // One a day at most. Two quirks landing on the same night reads as
        // a bug even when it is not, and the caller has one line to say.
        if (landed == Quirk::None) landed = q;
      }
    } else {
      // Slower than it filled: nearly becoming something and then not is a
      // real thing that happens, and it should cost you the progress
      // rather than the memory of it.
      held = std::max(0.0, held - dials.quirkDrainRate);
    }
  }
  return landed;
}

std::string QuirkLanded(Quirk q) {
  if (q == Quirk::None) return "";
  std::string s = "Somewhere in the last couple of seasons you became ";
  s += QuirkName(q);
  s += ". ";
  s += QuirkLine(q);
  return s;
}

bool Pick(Quirks& quirks, Quirk which) {
  // Picking Obsessive at the counter is not the same thing as becoming it,
  // and the difference is the only rule this file has.
  if (!IsPicked(which)) return false;
  if (quirks.picked != Quirk::None) return false;
  quirks.picked = which;
  if (!Has(quirks, which)) quirks.held.push_back(which);
  return true;
}

// --- What it all does --------------------------------------------------------

namespace {

// True if this trait is live: either you have it for good, or you are
// currently doing the thing. The one function that expresses the file's
// rule, so that no effect below can accidentally disagree with it.
//
// Returns the strength: 1 while it is a habit, `quirkStrength` once it has
// hardened, 0 otherwise.
double Live(const Quirks& quirks, const Logbook& record, Habit h, int today,
            const HabitDials& dials) {
  if (Has(quirks, HardensInto(h))) return dials.quirkStrength;
  return Doing(record, h, today, dials) ? 1.0 : 0.0;
}

// A multiplier scaled by how live the trait is. At strength 0 it is exactly
// 1, so an effect that is not happening costs nothing and rounds to
// nothing — the same invariant Phase 7's `built` flag exists to protect.
double By(double multiplier, double strength) {
  return 1.0 + (multiplier - 1.0) * strength;
}

}  // namespace

double HabitSkillGain(const Quirks& quirks, const Logbook& record, Skill lane,
                      int today, const HabitDials& dials) {
  double out = 1.0;
  const auto live = [&](Habit h) {
    return Live(quirks, record, h, today, dials);
  };

  // The approach is the training. Walking in at five, every day, in the
  // dark, with a rope on your back.
  if (lane == Skill::Endurance) out *= By(dials.lanePush, live(Habit::DawnPatrol));

  // A line you have hung on four hundred times teaches you exactly one
  // thing very well, and it is footwork.
  if (lane == Skill::Technique) out *= By(dials.lanePush, live(Habit::Grinder));
  // And a hundred lines once each teach you the other thing.
  if (lane == Skill::Head) out *= By(dials.lanePush, live(Habit::Tourist));

  // Straight onto the hard one gets you strong. It also gets you hurt, and
  // that is priced below.
  if (lane == Skill::Power) out *= By(dials.lanePush, live(Habit::NeverWarmsUp));

  // Plastic is a fingerboard with holds on it. It is also nothing like
  // rock, and the head is where that shows.
  if (lane == Skill::Fingers) out *= By(dials.lanePush, live(Habit::GymRat));
  if (lane == Skill::Head) out *= By(dials.laneStarve, live(Habit::GymRat));

  // The two ways to lead, and the head is the lane both of them are about.
  if (lane == Skill::Head) out *= By(dials.lanePush, live(Habit::RunsItOut));
  if (lane == Skill::Head) out *= By(dials.laneStarve, live(Habit::SewsItUp));
  // Walking away is the right call and it teaches you nothing, which is
  // the honest cost of the right call.
  if (lane == Skill::Head) out *= By(dials.laneStarve, live(Habit::KnowsWhenToStop));

  return out;
}

double HabitInjuryRisk(const Quirks& quirks, const Logbook& record, int today,
                       const HabitDials& dials) {
  double out = 1.0;
  const auto live = [&](Habit h) {
    return Live(quirks, record, h, today, dials);
  };
  out *= By(dials.riskPush, live(Habit::NeverWarmsUp));
  out *= By(dials.riskPush, live(Habit::RunsItOut));
  out *= By(dials.riskEase, live(Habit::SewsItUp));
  out *= By(dials.riskEase, live(Habit::KnowsWhenToStop));
  // Skin of Steel is deliberately absent. Skin is not a joint, and a habit
  // that made you harder to hurt because your tips are tough would be the
  // file quietly handing out a bonus for a thing that already has its own
  // effect -- see HabitSkinCost, which is the honest one.
  return out;
}

double HabitBetaRate(const Quirks& quirks, const Logbook& record, int today,
                     const HabitDials& dials) {
  double out = 1.0;
  out *= By(dials.betaPush, Live(quirks, record, Habit::Grinder, today, dials));
  out *= By(dials.betaStarve, Live(quirks, record, Habit::Tourist, today, dials));
  return out;
}

double HabitSkinCost(const Quirks& quirks, const Logbook& record, int today,
                     const HabitDials& dials) {
  return By(dials.skinEase,
            Live(quirks, record, Habit::SkinOfSteel, today, dials));
}

double HabitNerve(const Quirks& quirks, const Logbook& record, int today,
                  const HabitDials& dials) {
  double out = 0.0;
  out += dials.nerveBold * Live(quirks, record, Habit::RunsItOut, today, dials);
  out += dials.nerveShy *
         Live(quirks, record, Habit::KnowsWhenToStop, today, dials);
  // Picked, and it is the point of picking it.
  if (Has(quirks, Quirk::Stubborn)) out += dials.nerveBold;
  return out;
}

double HabitShiftPay(const Quirks& quirks) {
  // The picked lane. Gregarious is worth something at the Lot and costs you
  // at the counter, because the shifts you get are the shifts you turn up
  // for and you are always somewhere else.
  double out = 1.0;
  if (Has(quirks, Quirk::Gregarious)) out *= 0.92;
  if (Has(quirks, Quirk::Quiet)) out *= 1.05;
  return out;
}

double HabitDailyCost(const Quirks& quirks) {
  double out = 1.0;
  if (Has(quirks, Quirk::BadWithMoney)) out *= 1.12;
  return out;
}

std::string HowYouClimb(const Quirks& quirks, const Logbook& record, int today,
                        const HabitDials& dials) {
  // Two questions, answered separately on purpose: what you have been doing
  // lately, and what you turned out to be. A game that ran them together
  // would be saying that a phase and a person are the same thing, which is
  // the one thing this file exists to deny.
  const std::vector<Habit> now = HabitsNow(record, today, dials);

  std::string s;
  if (now.empty()) {
    s = "Lately you have been climbing the way most people climb.";
  } else {
    s = HabitLine(now.front());
  }

  if (!quirks.held.empty()) {
    s += " And by now you are ";
    for (std::size_t i = 0; i < quirks.held.size(); i++) {
      if (i > 0) s += i + 1 == quirks.held.size() ? " and " : ", ";
      s += QuirkName(quirks.held[i]);
    }
    s += ".";
  }
  return s;
}

}  // namespace dirtbag

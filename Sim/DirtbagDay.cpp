#include "DirtbagDay.h"

#include "DirtbagBody.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

namespace {

// Which skills a route type trains — mirrors the resolver's BlendedSkill
// question ("what does this hold ask for?") from the training side.
struct TrainingWeights {
  double power = 0.0, fingers = 0.0, technique = 0.0;
};

TrainingWeights WeightsFor(RouteType type) {
  switch (type) {
    case RouteType::Crimp:     return {0.1, 0.7, 0.2};
    case RouteType::Power:     return {0.6, 0.1, 0.3};
    case RouteType::Endurance: return {0.2, 0.2, 0.2};  // endurance handled below
    case RouteType::Technical: return {0.1, 0.2, 0.7};
    case RouteType::Dyno:      return {0.7, 0.1, 0.2};
    case RouteType::Crack:     return {0.2, 0.2, 0.6};
  }
  return {0.2, 0.2, 0.2};
}

void Gain(double& skill, double amount) {
  skill = std::min(100.0, skill + amount);
}

}  // namespace

void Charge(PlayerState& player, double amount) {
  if (amount <= 0.0) return;
  const double paid = std::min(player.cash, amount);
  player.cash -= paid;
  player.owed += amount - paid;
}

void Pay(PlayerState& player, double amount) {
  if (amount <= 0.0) return;
  const double toDebt = std::min(player.owed, amount);
  player.owed -= toDebt;
  player.cash += amount - toDebt;
}

bool WorkOddJob(PlayerState& player, DayState& day, const OddJob& job,
                const DayDials& dials) {
  if (job.needsVan && !VanRuns(player.van)) return false;

  PassHours(day, job.hours, dials);
  day.energy = std::max(0.0, day.energy - job.energy);
  Pay(player, job.pay);
  player.job.daysWorked++;

  // What the work says about you. The board was written with these in
  // mind — the best-paying gig on it is the one that costs you the most.
  if (job.name.find("guidebook") != std::string::npos) {
    TookTheGuidebookPhotos(player.standing);
  } else if (job.name.find("trail work") != std::string::npos) {
    DidTrailWork(player.standing);
  } else if (job.name.find("setting") != std::string::npos) {
    SetAtTheGym(player.standing);
  }
  return true;
}

void WorkSalariedDay(PlayerState& player, DayState& day, const JobDials& jobs,
                     const DayDials& dials) {
  // The hours are the mechanic. Nine to five means the clock arrives at the
  // far side of the day having skipped everything the day was for.
  const double until = jobs.salaryStartHour + jobs.salaryHours;
  if (day.hour < until) PassHours(day, until - day.hour, dials);
  day.energy = std::max(0.0, day.energy - jobs.salaryEnergy);
  Pay(player, SalaryDayPay(jobs));
  player.job.daysWorked++;
}

void TakeSalariedJob(PlayerState& player) { player.job.salaried = true; }

void QuitSalariedJob(PlayerState& player, const JobDials& jobs) {
  if (!player.job.salaried) return;
  player.job.salaried = false;
  // Walking out costs something, but not much: the job was the punishment.
  player.climber.psyche =
      std::max(0.05, player.climber.psyche - jobs.salaryQuitPsycheCost);
}

DayState WakeUp(const PlayerState& player, const DayDials& dials) {
  (void)player;
  DayState day;
  day.hour = dials.wakeHour;
  day.energy = 100.0;
  day.hunger = 0.0;
  return day;
}

void PassHours(DayState& day, double hours, const DayDials& dials) {
  if (hours <= 0.0) return;
  day.hour += hours;
  day.hunger = std::min(100.0, day.hunger + dials.hungerPerHour * hours);
}

void Rest(DayState& day, double hours, const DayDials& dials) {
  if (hours <= 0.0) return;
  PassHours(day, hours, dials);
  day.energy = std::min(100.0, day.energy + dials.restEnergyPerHour * hours);
}

bool EatMeal(PlayerState& player, DayState& day, const DayDials& dials) {
  if (player.cash < dials.mealCost) return false;
  player.cash -= dials.mealCost;
  day.hunger = std::max(0.0, day.hunger - dials.mealHunger);
  PassHours(day, 0.5, dials);
  return true;
}

void WorkShift(PlayerState& player, DayState& day, const DayDials& dials) {
  // Debt first: a wage does not reach your pocket until you are level.
  Pay(player, dials.shiftWage);
  day.energy = std::max(0.0, day.energy - dials.shiftEnergy);
  PassHours(day, dials.shiftHours, dials);
}

Climber ClimberForSession(const PlayerState& player, const DayState& day,
                          const DayDials& dials) {
  Climber c = player.climber;
  // Running on empty shows up as nerve before it shows up as strength, and
  // it fades in rather than snapping: the eighth burn of the day is worse
  // than the second even when nothing has "run out" yet.
  const double fatigue =
      Clamp01((dials.freshEnergy - day.energy) / std::max(1.0, dials.freshEnergy));
  c.psyche = std::max(0.05, c.psyche - dials.fatiguePsyche * fatigue);

  // The dog, which cuts both ways: a settled one at the base of the boulder
  // is worth a little, and knowing it has not eaten is worth rather more in
  // the other direction. Applied here because this is where the body you
  // actually climb in gets assembled.
  c.psyche = Clamp01(c.psyche + DogPsyche(player.dog));
  c.psyche = std::max(0.05, c.psyche);
  return c;
}

void StartGymSession(PlayerState& player, DayState& day, const DayDials& dials) {
  day.session = StartSession(ClimberForSession(player, day, dials));
  // Whatever is on your feet comes with you. Shoes live on the career
  // rather than the body, so the session has to be handed them.
  day.session.shoeWear = player.shoes.wear;
  // What you dragged up the hill. GoToTheGym overrides this afterwards,
  // because indoors the landing is somebody else's problem.
  day.session.padding = PaddingFrom(player.kit, KitDials{});
  day.atGym = true;
}

bool GoToTheGym(PlayerState& player, DayState& day, const KitDials& kit,
                const DayDials& dials) {
  if (!IsGymMember(player.kit)) return false;
  PassHours(day, kit.gymTravelHours, dials);
  StartGymSession(player, day, dials);
  // Full mats, every time. This is what you are actually paying for on the
  // days the weather has already decided for you.
  day.session.padding = 1.0;
  return true;
}

bool HangboardSession(PlayerState& player, DayState& day, const KitDials& kit,
                      const DayDials& dials) {
  if (!player.kit.hangboard || day.hangboardDone) return false;
  // Hanging on skin that is already gone is how you take a week off, and a
  // policy that cannot see that would train straight through the injury the
  // game does not model yet.
  if (player.climber.skin <= kit.hangboardSkinCost) return false;

  PassHours(day, kit.hangboardHours, dials);
  day.hangboardDone = true;
  day.energy = std::max(0.0, day.energy - kit.hangboardEnergy);
  player.climber.skin -= kit.hangboardSkinCost;
  // Fingery by definition and at full intensity — nobody hangs a board
  // casually. This is where the load a season of training costs comes from,
  // and it is more than a four-burn session by design.
  AddLoad(player.climber, kit.hangboardLoad);
  Gain(player.climber.skills.fingers,
       kit.hangboardFingerGain *
           std::max(0.15, 1.0 - player.climber.skills.fingers /
                                    dials.trainingCeiling));
  return true;
}

ProjectMemory& MemoryFor(PlayerState& player, const Route& route) {
  for (ProjectMemory& mem : player.projects) {
    if (mem.routeName == route.name) return mem;
  }
  player.projects.emplace_back();
  player.projects.back().routeName = route.name;
  return player.projects.back();
}

void ApplyAttemptToDay(PlayerState& player, DayState& day, const Route& route,
                       const AttemptResult& result, const Rng& worldRng,
                       const DayDials& dials) {
  PassHours(day, dials.attemptHours, dials);

  // Trying hard costs more than cruising: how far the line is above you,
  // measured the same way the wall measures it.
  const double over = std::max(
      0.0, static_cast<double>(route.trueGrade) -
               AbilityOnRoute(player.climber, route));
  day.energy = std::max(
      0.0, day.energy - dials.attemptEnergy - dials.attemptEnergyPerGrade * over);

  if (result.timeline.empty()) return;

  // Rubber goes by the move, and faster the harder you pull.
  WearShoes(player.shoes, static_cast<int>(result.timeline.size()),
            route.trueGrade);

  // Training creep: challenge relative to what the route asks of you.
  // Two grades below you trains nothing; at your level trains most of the
  // rate; above you trains the full rate — you get strong by trying hard.
  // The skill→grade mapping is the resolver's own (SkillToGrade), so "at
  // your level" means the same thing to the trainer as to the wall.
  const TrainingWeights w = WeightsFor(route.type);
  const Skills& s = player.climber.skills;
  const double routeAsk =
      w.power * s.power + w.fingers * s.fingers + w.technique * s.technique;
  const double skillGrade = SkillToGrade(routeAsk);
  const double challenge =
      Clamp01((static_cast<double>(route.trueGrade) - skillGrade + 2.0) / 3.0);

  // Tendons keep their own ledger, on that same number — two ways of asking
  // "was that hard" would drift, and the drift would be invisible until
  // somebody got hurt for no reason. The hardest hold on the route stands
  // for what it asked of your fingers, because one crimp in a route of jugs
  // is still the move that hurt you.
  HoldType hardest = HoldType::Jug;
  double worst = -1.0;
  for (const Move& m : route.moves) {
    if (m.difficulty > worst) { worst = m.difficulty; hardest = m.hold; }
  }
  AccrueLoad(player.climber, challenge, hardest);

  // And if something is already wrong, this is the burn that either got
  // away with it or did not. Keyed on the session's attempt count so that
  // twenty burns are twenty separate gambles rather than one.
  if (IsHurt(player.climber)) {
    ClimbOnIt(player.climber, worldRng, player.day, day.session.attemptsMade);
  }

  // Diminishing returns: the same session that builds a beginner barely
  // moves a veteran. Newcomers feel progress; the top of the range is a
  // grind, which is the honest version of a climbing career.
  const auto headroom = [&](double skill) {
    return std::max(0.15, 1.0 - skill / dials.trainingCeiling);
  };
  // How far you actually got, which is what separates working a line from
  // flailing at one.
  const double reached =
      route.moves.empty()
          ? 0.0
          : static_cast<double>(result.highpoint) /
                static_cast<double>(route.moves.size());
  const double engagement =
      dials.engagementFloor + (1.0 - dials.engagementFloor) * Clamp01(reached);

  const double amount = dials.trainingRate * challenge * engagement;

  Gain(player.climber.skills.power,
       amount * w.power * 3.0 * headroom(s.power));
  Gain(player.climber.skills.fingers,
       amount * w.fingers * 3.0 * headroom(s.fingers));
  Gain(player.climber.skills.technique,
       amount * w.technique * 3.0 * headroom(s.technique));
  // Endurance trains by mileage — moves climbed, whatever the grade — but
  // mileage is a slower teacher than trying hard, so it stays below the
  // targeted skills rather than outrunning them.
  Gain(player.climber.skills.endurance,
       dials.trainingRate * dials.enduranceMileageRate *
           static_cast<double>(result.timeline.size()) *
           headroom(s.endurance));
}

void SleepToNextDay(PlayerState& player, DayState& day, const Rng& worldRng,
                    const DayDials& dials) {
  // The wall's tab comes home: today's remaining skin is tomorrow's start.
  if (day.atGym) {
    player.climber.skin = day.session.skinLeft;
    player.climber.psyche = day.session.psyche;
  }
  player.climber.skin =
      std::min(dials.maxSkin, player.climber.skin + dials.skinRegenPerNight);
  player.climber.psyche +=
      (dials.psycheBaseline - player.climber.psyche) * dials.psycheHomeRate;

  // Tendons recover on their own clock — a month where skin takes six
  // nights. A day you never pulled on is worth about twice one you did,
  // which is what finally makes a rest day a decision.
  BodyDay(player.climber, !day.atGym);
  // And the roll, on a day you actually pulled on. Never on a rest day:
  // tendons do not tear in a camp chair.
  if (day.atGym) RollForInjury(player.climber, worldRng, player.day);

  // The month runs down like everything else that runs out. Without this
  // the probe reported 365 days of membership bought with a single $75,
  // which is a very good gym.
  KitDay(player.kit);

  player.day += 1;
  // Bills land on their morning, every billsEveryDays-th day after day 1.
  if (dials.billsEveryDays > 0 && player.day > 1 &&
      (player.day - 1) % dials.billsEveryDays == 0) {
    Charge(player, dials.billsAmount);
  }

  // A hungry night is a bad night: recovery scales down toward the floor.
  const double hungerPenalty =
      Clamp01((day.hunger - dials.starvingHunger) / (100.0 - dials.starvingHunger));
  const double recovered =
      100.0 - (100.0 - dials.sleepEnergyFloor) * hungerPenalty;

  day = DayState{};
  day.hour = dials.wakeHour;
  day.energy = recovered;
}

CareerSummary SummarizeCareer(const PlayerState& player) {
  CareerSummary out;
  // Ability is the all-round read: the wall weights skills per hold, but a
  // career card wants one honest number.
  const Skills& s = player.climber.skills;
  out.abilityGrade =
      SkillToGrade((s.power + s.fingers + s.technique + s.endurance) / 4.0);

  for (const ProjectMemory& m : player.projects) {
    out.totalAttempts += m.attempts;
    if (m.sent) {
      out.totalSends++;
      if (m.grade > out.hardestSendGrade) {
        out.hardestSendGrade = m.grade;
        out.hardestSendName = m.routeName;
        out.hardestSendStyle = m.firstSendStyle;
      }
    } else if (m.attempts > 0) {
      out.openProjects++;
      if (m.attempts > out.nemesisAttempts) {
        out.nemesisAttempts = m.attempts;
        out.nemesis = m.routeName;
      }
    }
  }
  return out;
}

std::string CareerLine(const CareerSummary& career) {
  std::string line = "You climb ";
  line += BoulderGradeName(static_cast<int>(career.abilityGrade));
  line += ".";

  if (career.hardestSendGrade >= 0) {
    line += "  Hardest: " + career.hardestSendName + " (" +
            BoulderGradeName(career.hardestSendGrade) + ").";
  } else if (career.totalAttempts > 0) {
    line += "  Nothing sent yet.";
  }

  if (!career.nemesis.empty() && career.nemesisAttempts > 3) {
    line += "  " + career.nemesis + " has taken " +
            std::to_string(career.nemesisAttempts) + " burns and counting.";
  }
  return line;
}

std::vector<Route> GymBoard(const Rng& worldRng, int count) {
  // Setter-voice names; the board cycles through them as it grows.
  static const char* kNames[] = {
      "Jug Haul",        "Slab of Regret",  "Pink Crimps",   "Dyno Tax",
      "Setter's Revenge", "Volume Country", "Campus Special", "The Blue One",
      "Corner Office",   "Screwed-On Feet", "Pinch City",    "Reset Tuesday",
      "Moonboard Refugee", "The Traverse",  "Cave Problem",  "The Fifty"};
  constexpr int kNameCount = static_cast<int>(sizeof(kNames) / sizeof(kNames[0]));
  static const RouteType kTypes[] = {RouteType::Endurance, RouteType::Technical,
                                     RouteType::Crimp,     RouteType::Power,
                                     RouteType::Dyno,      RouteType::Crimp,
                                     RouteType::Power,     RouteType::Technical};
  constexpr int kTypeCount = static_cast<int>(sizeof(kTypes) / sizeof(kTypes[0]));

  Rng board = worldRng.Derive("gym-board");
  std::vector<Route> routes;
  routes.reserve(count);
  for (int i = 0; i < count; i++) {
    const int grade = i;  // a clean ladder: V0, V1, ... up the board
    // Setters are people too: roughly one problem in six wears a soft tag
    // over a harder truth.
    const int trueGrade = board.Chance(1.0 / 6.0) ? grade + 1 : grade;
    routes.push_back(BuildRoute(worldRng, kNames[i % kNameCount], grade,
                                trueGrade, kTypes[i % kTypeCount],
                                Discipline::Boulder));
  }
  return routes;
}

}  // namespace dirtbag

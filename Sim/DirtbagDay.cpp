#include "DirtbagDay.h"

#include "DirtbagBody.h"
#include "DirtbagNarrator.h"
#include "DirtbagSport.h"

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
                const Rng& worldRng, bool theHardWay, const DayDials& dials) {
  if (job.needsVan && !VanRuns(player.van)) return false;
  const Craft craft = CraftForGig(job.name);
  // They will not have you. **A sacking has to show up as work you cannot
  // take**, and the board is filtered for the player -- but the rule has
  // to be here too, or a caller that skipped the filter walks straight
  // past a sacking.
  if (!WillTheyHireYou(player.hand, craft)) return false;

  PassHours(day, job.hours, dials);
  day.energy = std::max(0.0, day.energy - job.energy);

  // **Whatever came up on the shift**, and what you did about it. Rolled
  // before the pay, because what it pays depends on how it went.
  const ShiftMoment moment = MomentOnShift(craft, worldRng, player.day);
  const MomentOutcome went =
      DecideTheMoment(player.hand, moment, theHardWay, worldRng, player.day);

  // The trade, and the very little it teaches your climbing.
  //
  // **Through the same diminishing returns everything else trains
  // through**, and clamped. The first version added the gain raw, which
  // meant work was the one training path in the game with no headroom on
  // it -- thirty years of shifts is about seven thousand hours, and at a
  // flat rate that is a hundred and thirty skill points into a stat that
  // stops at a hundred. Work that out-trains climbing would make the
  // salaried trap not a trap, and a skill that runs past its ceiling is a
  // bug in any case.
  const WorkedShift worked = WorkTheTrade(player.hand, craft, job.hours);
  double* lane = &player.climber.skills.technique;
  switch (worked.teaches) {
    case Skill::Power: lane = &player.climber.skills.power; break;
    case Skill::Fingers: lane = &player.climber.skills.fingers; break;
    case Skill::Endurance: lane = &player.climber.skills.endurance; break;
    case Skill::Head: lane = &player.climber.skills.head; break;
    case Skill::Technique:
    default: break;
  }
  Gain(*lane, worked.skillGain *
                  std::max(0.15, 1.0 - *lane / dials.trainingCeiling));

  // What the work pays *you*. Three different things meet here: the
  // origin's CV, which is a fact; how much of a purist you are, which is a
  // choice; and **how good you are at the job**, which is the only one of
  // the three you can do anything about.
  Pay(player, job.pay * ShiftPayMultiplier(player.character) *
                  HabitShiftPay(player.quirks) *
                  CraftPay(player.hand, craft) * went.payMultiplier);
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

void TakeSalariedJob(PlayerState& player) {
  // The streak dies at the signature, not at the first shift. You knew what
  // you were doing when you shook hands.
  BreakTheStreak(player.job);
  player.job.salaried = true;
}

void QuitSalariedJob(PlayerState& player, const JobDials& jobs) {
  if (!player.job.salaried) return;
  player.job.salaried = false;
  // Walking out costs something, but not much: the job was the punishment.
  player.climber.psyche =
      std::max(0.05, player.climber.psyche - jobs.salaryQuitPsycheCost);
}

DayState WakeUp(const PlayerState& player, const DayDials& dials) {
  DayState day;
  day.hour = dials.wakeHour;
  day.energy = 100.0;
  day.hunger = 0.0;

  // **You do not wake up at a hundred when you are ill.** A night's sleep
  // gives back what it gives back, and a body fighting something gives
  // back less -- which is the difference between being ill and being
  // tired, and the reason a week of it costs a week rather than a lie-in.
  const AilmentDials ad;
  if (player.sickness.active) {
    day.energy = std::min(day.energy, ad.sickEnergyCeiling);
  }
  // And an abscess is the main thing about your week.
  if (player.teeth.stage == ToothStage::Abscess) {
    day.energy = std::max(0.0, day.energy - ad.toothAbscessEnergy);
  }
  return day;
}

void PassHours(DayState& day, double hours, const DayDials& dials) {
  if (hours <= 0.0) return;
  day.hour += hours;
  day.hunger = std::min(100.0, day.hunger + dials.hungerPerHour * hours);
}

void Rest(DayState& day, double hours, const Life& life,
          const DayDials& dials) {
  if (hours <= 0.0) return;
  PassHours(day, hours, dials);
  // What an hour of sitting still is worth depends on what you are doing
  // with it. Somebody halfway through a paperback is resting; somebody
  // staring at the line is not.
  day.energy = std::min(100.0, day.energy + dials.restEnergyPerHour *
                                                LifeRestRate(life) * hours);
}

bool EatMeal(PlayerState& player, DayState& day, const DayDials& dials) {
  // A stove and the will to use it: cheaper, and it goes further. Both
  // scaled by how much you have actually been cooking, so the discount is
  // a habit rather than a purchase.
  // What being a regular is worth off the bill. Small on purpose -- this
  // is a relationship, not a loyalty card.
  const double cost = dials.mealCost * LifeMealCost(player.life) *
                      LocalPrice(player.locals, Service::Meal);
  if (player.cash < cost) {
    // **And they saw it.** Between you and whoever was standing there;
    // this is the one thing in the file the town does not get told.
    TellOne(player.locals, Service::Meal, Heard::Broke, "", player.day);
    return false;
  }
  if (Local* who = At(player.locals, Service::Meal)) {
    day.heard = WhatTheySay(*who, player.day);
    Seen(*who, player.day);
  }
  player.cash -= cost;
  day.hunger = std::max(
      0.0, day.hunger - dials.mealHunger * LifeMealHunger(player.life));
  PassHours(day, 0.5, dials);
  return true;
}

bool SpendTheEvening(PlayerState& player, DayState& day, Thread what,
                     const DayDials& dials) {
  const double hours = AsksFor(what);
  if (hours <= 0.0) return false;
  // One path in, so that starting a thing and keeping it going cost the
  // same hours. Two entry points is how the first one came to be free.
  if (!Going(player.life, what) &&
      !TakeUp(player.life, what, player.day)) {
    return false;
  }
  if (!GiveItTime(player.life, what, hours, player.day)) return false;
  PassHours(day, hours, dials);
  // Busking pays after the fact, off the warmth the hours just bought --
  // an afternoon that went well is an afternoon that paid. Debt first,
  // like every other dollar in this game.
  const double busked = BuskingPay(player.life, hours);
  if (busked > 0.0) Pay(player, busked);
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

void StartGymSession(PlayerState& player, DayState& day, const KitDials& kit,
                     const DayDials& dials) {
  day.session = StartSession(ClimberForSession(player, day, dials));
  // Whatever is on your feet comes with you. Shoes live on the career
  // rather than the body, so the session has to be handed them.
  day.session.shoeWear = player.shoes.wear;
  // What you dragged up the hill. GoToTheGym overrides this afterwards,
  // because indoors the landing is somebody else's problem.
  day.session.padding = PaddingFrom(player.kit, kit);
  // And the rack, for the same reason and by the same rule: what you
  // carried in. GoToTheGym does not override it — there is nothing indoors
  // to place it on, and a trad route in a gym is not a thing.
  day.session.rack = player.rack;
  // And who is climbing, which is the same rule again: what you walked in
  // with. Read once at the start of the session rather than per burn,
  // because a habit does not change between goes.
  day.session.betaRate =
      HabitBetaRate(player.quirks, player.logbook, player.day);
  day.session.skinRate =
      HabitSkinCost(player.quirks, player.logbook, player.day);
  day.atGym = true;
  day.indoors = false;   // GoToTheGym says otherwise, and it is the only thing that does
}

bool GoToTheGym(PlayerState& player, DayState& day, const KitDials& kit,
                const DayDials& dials) {
  if (!IsGymMember(player.kit)) return false;
  PassHours(day, kit.gymTravelHours, dials);
  StartGymSession(player, day, kit, dials);
  // Full mats, every time. This is what you are actually paying for on the
  // days the weather has already decided for you.
  day.session.padding = 1.0;
  day.indoors = true;
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
  player.projects.back().discipline = route.discipline;
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

  // **What gets around.** Here rather than anywhere else for the reason
  // the logbook is here: this is the one function every burn in the game
  // passes through. The newest send replaces the last, which is exactly
  // the question a regular is answering -- *what did you do last time.*
  if (result.sent) {
    Tell(player.locals, Heard::Sent, route.name, player.day);
  }

  // **What sort of go that was.** The instrumentation Phase 7 said this
  // needed and did not have: the resolver knows what happened on one route
  // and the ledger knows what has happened on one line, and until now
  // nothing anywhere could answer *what sort of climber is this*.
  //
  // Here rather than in the session loop because this is the one function
  // every burn in the game passes through -- the batch loop, the live
  // attempt, the engine's own commit -- and because it is the only one that
  // has the whole career in its hands.
  {
    Logbook& book = player.logbook;
    if (day.firstPullOnHour < 0.0) day.firstPullOnHour = day.hour;
    Note(book, Did::Burn, 1.0, player.day);

    // Your limit, judged the way the guidebook judges it, so "never warms
    // up" means the same thing to the logbook as to the player reading the
    // line from the ground.
    const RouteRead read = ReadRoute(player.climber, route);
    if (read == RouteRead::AtYourLimit || read == RouteRead::Project ||
        read == RouteRead::NotThisYear) {
      Note(book, Did::BurnAtYourLimit, 1.0, player.day);
    }
    // The session advice's own number, read rather than copied: "climbs on
    // nothing" and "your tips are gone" have to mean the same thing.
    if (day.session.skinLeft <= SessionLoopDials{}.thinSkin) {
      Note(book, Did::BurnOnThinSkin, 1.0, player.day);
    }

    // The line, and whether it is a new one. `attempts` has not been
    // incremented for this burn yet on the live path, so the ledger is
    // read before the commit rather than after -- a first burn read
    // afterwards is never the first burn.
    const ProjectMemory& mem = MemoryFor(player, route);
    if (mem.attempts <= 1) Note(book, Did::LineTouched, 1.0, player.day);
    if (mem.attempts >= dials.grindingAfter) {
      Note(book, Did::BurnOnOneLine, 1.0, player.day);
    }

    // And how it was led, which only means anything on gear.
    if (route.discipline == Discipline::Trad) {
      const int moves = static_cast<int>(result.timeline.size());
      Note(book, Did::MoveOnGear, moves, player.day);
      int pieces = 0, unprotected = 0;
      for (int i = 0; i < moves; i++) {
        if (i < static_cast<int>(result.gear.quality.size()) &&
            result.gear.quality[i] > 0.0) {
          pieces++;
        }
        if (!OnTheRope(route, i, SportDials{}, result.gear)) unprotected++;
      }
      Note(book, Did::PiecePlaced, pieces, player.day);
      Note(book, Did::MoveRunOut, unprotected, player.day);
    }
  }

  // **And what that was, in words.** Here rather than at any of the three
  // call sites above it, because this is the one function every burn in the
  // game passes through -- the batch loop, the live attempt the minigame
  // drives, and the engine's own commit -- and a line the player reads once
  // must not depend on which of them remembered to ask for it.
  {
    AttemptInput told;
    told.climber = player.climber;
    told.route = route;
    told.warmth = day.session.warmth;
    told.shoeWear = day.session.shoeWear;
    told.padding = day.session.padding;
    told.rack = day.session.rack;
    const ProjectMemory& mem = MemoryFor(player, route);
    told.attemptNumber = mem.attempts;
    told.beta = mem.beta;
    told.cleanliness = mem.cleanliness;
    // body-ok: nothing is resolved from this. It is the narrator's copy of
    // what the burn was climbed under, and the burn has already happened --
    // stamping a body on it would price an attempt that is over.
    day.lastBurn = HowItWent(told, result);
  }

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
  } else {
    // Or nothing was wrong, and this is the burn where something goes. The
    // acute path: a cold crimp at your limit, one move, done. Warmth is
    // read from the session because the cold first burn is the classic.
    // **What the joint has been through rides on this roll.** Cortisone
    // in it and old scars on it both make the same crimp more likely to
    // go, which is the whole of what "the body keeps score" means
    // mechanically -- see Sim/DirtbagMedical.h.
    const bool wasFine = !IsHurt(player.climber);
    TweakSomething(player.climber, worldRng, player.day,
                   day.session.attemptsMade, challenge, hardest,
                   day.session.warmth, BodyDials{}, AgeDials{},
                   InjuryRiskMultiplier(player.character) *
                       // And how you have been climbing, which is the one
                       // of these three you chose by doing rather than by
                       // being born or by paying.
                       HabitInjuryRisk(player.quirks, player.logbook,
                                       player.day) *
                       BodyRisk(player.medical, player.day) *
                       // **Twenty minutes of a morning, and the only thing
                       // in the medical half of this game that makes the
                       // odds better rather than worse.** Boring, it
                       // works, and nobody does it.
                       PrehabRisk(player.upkeep));
    if (wasFine && IsHurt(player.climber)) {
      StartComeback(player.medical, player.climber, player.day);
    }
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

  // **What a session teaches you depends on who you are.** Talents (the two
  // you were born with and may not know about), the flaw you picked, your
  // origin and how disciplined you are all land here, per lane, as one
  // multiplier -- so this is the single seam where identity meets progress
  // rather than eleven scattered ones.
  //
  // `WorkedOn` is called with the same amount, because the thing that
  // surfaces a talent has to be the thing the talent affects: a gift in a
  // lane you never train stays a secret forever, which is the design.
  const bool indoor = day.atGym;
  const auto teach = [&](Skill lane, double base) {
    const double mult = SkillGainMultiplier(player.character, lane, indoor,
                                           false, player.climber.psyche) *
                        HabitSkillGain(player.quirks, player.logbook, lane,
                                       player.day);
    WorkedOn(player.character, lane, base > 0.0 ? 1.0 : 0.0);
    return base * mult;
  };

  Gain(player.climber.skills.power,
       teach(Skill::Power, amount * w.power * 3.0 * headroom(s.power)));
  Gain(player.climber.skills.fingers,
       teach(Skill::Fingers, amount * w.fingers * 3.0 * headroom(s.fingers)));
  Gain(player.climber.skills.technique,
       teach(Skill::Technique,
             amount * w.technique * 3.0 * headroom(s.technique)));
  // Endurance trains by mileage — moves climbed, whatever the grade — but
  // mileage is a slower teacher than trying hard, so it stays below the
  // targeted skills rather than outrunning them.
  Gain(player.climber.skills.endurance,
       teach(Skill::Endurance,
             dials.trainingRate * dials.enduranceMileageRate *
                 static_cast<double>(result.timeline.size()) *
                 headroom(s.endurance)));

  // And head, on the boldest thing you committed to rather than the hardest.
  // Falling counts: this reads the highpoint reached, not whether it went,
  // because a fall from above the bolt teaches the lesson at least as well
  // as sticking the move did. Head was read by the resolver, by the sport
  // runout, and by the age model that calls it one of two skills that never
  // decline, and until now nothing in the game trained it at all — a whole
  // axis frozen at whatever the climber was born with.
  const int reachedIndex =
      std::min(result.highpoint, static_cast<int>(route.moves.size()) - 1);
  double boldest = 0.0;
  for (int i = 0; i <= reachedIndex; i++) {
    // Reading the gear that actually went in, which is the difference
    // between training head on a pitch you sewed up and one you ran out.
    // Empty on everything but a trad lead, where the sport bolts and the
    // boulder's ground answer for themselves.
    boldest = std::max(boldest, ExposureAt(route, i, day.session.padding,
                                           SessionDials{}, result.gear));
  }
  Gain(player.climber.skills.head,
       teach(Skill::Head,
             dials.headExposureRate * boldest * headroom(s.head)));
}

void SleepToNextDay(PlayerState& player, DayState& day, const Rng& worldRng,
                    const DayDials& dials) {
  // **What kind of day that was.** Counted at the end because that is the
  // only point where the answer is known -- whether you got on anything is
  // not a thing you can ask at breakfast.
  // A day out is a day you got on something. A session that started and
  // produced nothing is not a day out and is not a bail either -- it is the
  // rock not being in condition, which happens constantly here and is not a
  // fact about the climber.
  if (day.atGym && day.firstPullOnHour >= 0.0) {
    Note(player.logbook, Did::DayOut, 1.0, player.day);
    if (day.indoors) Note(player.logbook, Did::DayIndoors, 1.0, player.day);
    if (day.firstPullOnHour <= dials.dawnBefore) {
      Note(player.logbook, Did::DawnStart, 1.0, player.day);
    }
    // **Went home with something left**, which is a decision rather than a
    // forecast. Read against the same thin-skin line the session advice
    // uses to say "your tips are gone; jugs or go home" -- somebody who
    // stops above it stopped because they chose to.
    if (day.session.skinLeft > SessionLoopDials{}.thinSkin) {
      Note(player.logbook, Did::StoppedEarly, 1.0, player.day);
    }
  } else {
    // A day you did not climb still passes, and the logbook has to know --
    // otherwise a winter off reads as a winter of whatever you were doing
    // in the autumn, forever, because nothing decayed it.
    RememberTo(player.logbook, player.day);
  }
  // And whether any of it has been going on long enough to have stopped
  // being something you do.
  player.becameToday = HabitsDay(player.quirks, player.logbook, player.day);

  // **And the life outside it, which cools whether or not you climbed.**
  // Here with everything else that counts down at night, for the reason
  // this file has now written four times: three per-day rules in this
  // project were written and left uncalled, and every one was found late.
  player.lostToday = LifeNight(player.life, player.day);

  // **And the town.** Seeded here rather than at creation so that a save
  // written before there were any people in it loads into a town that has
  // them -- the same lazy-open the circuit uses two hundred lines below,
  // and for the same reason: a thing that should always exist is better
  // opened by the tick than by remembering to call something.
  if (player.locals.people.empty()) {
    player.locals = TheLocals(DirtbagTown(), worldRng);
  }
  LocalsDay(player.locals, player.day);

  // The two facts with nowhere else to be noticed from. A send goes
  // through `ApplyAttemptToDay`, a first ascent through `ClaimFirstAscent`
  // and an injury through the roll below; a season title and a signature
  // are assembled by hand at two call sites each, so this is the only
  // place both the game and the probe pass through. See the note on
  // `Locals::knownTitles`.
  if (player.circuit.titles > player.locals.knownTitles) {
    player.locals.knownTitles = player.circuit.titles;
    Tell(player.locals, Heard::Won, "the season", player.day);
  }
  if (static_cast<int>(player.sponsor.tier) > player.locals.knownTier) {
    player.locals.knownTier = static_cast<int>(player.sponsor.tier);
    Tell(player.locals, Heard::Sponsored, "", player.day);
  }

  // The wall's tab comes home: today's remaining skin is tomorrow's start.
  if (day.atGym) {
    player.climber.skin = day.session.skinLeft;
    player.climber.psyche = day.session.psyche;
  }
  player.climber.skin =
      std::min(dials.maxSkin, player.climber.skin + dials.skinRegenPerNight +
                                  SkinBonus(player.dreams));
  // Overnight, psyche drifts back toward where it lives. **Where it lives
  // is not a constant** -- an hour of talking about it moves the baseline
  // for a month, which is the one thing a rest day cannot do and the whole
  // reason the shrink exists as a purchase rather than a rest.
  //
  // Nor is it only about climbing. Somebody who does not care what you
  // climbed today moves where psyche lives; so, downward, does the month
  // after they stop being somebody. See Sim/DirtbagLife.h.
  const double homeTo =
      Clamp01(PsycheFloor(player.upkeep, dials.psycheBaseline, player.day) +
              LifePsyche(player.life));
  player.climber.psyche +=
      (homeTo - player.climber.psyche) * dials.psycheHomeRate;

  // Tendons recover on their own clock — a month where skin takes six
  // nights. A day you never pulled on is worth about twice one you did,
  // which is what finally makes a rest day a decision.
  BodyDay(player.climber, !day.atGym, AgeOn(player.day));
  // And the roll, on a day you actually pulled on. Never on a rest day:
  // tendons do not tear in a camp chair.
  if (day.atGym) {
    const bool wasFine = !IsHurt(player.climber);
    RollForInjury(player.climber, worldRng, player.day);
    if (wasFine && IsHurt(player.climber)) {
      // A fresh one starts its comeback at stage one, undiagnosed. Here
      // rather than inside `RollForInjury`, because `DirtbagBody` is
      // engine-free of *this* too: it decides that you are hurt, and what
      // you do about it is a different file.
      StartComeback(player.medical, player.climber, player.day);
      // And the town notices you limping before you tell anybody. Here
      // rather than at the other `StartComeback` because that one is the
      // in-session roll and this is the night's -- and a person who saw
      // you at the crag this afternoon is not who this is about.
      Tell(player.locals, Heard::Hurt,
           InjuryName(player.climber.injury.kind), player.day);
    }
  }

  // The comeback's own clock, the scars fading, and the premium. Here with
  // everything else that counts down at night.
  MedicalDay(player.medical, player.climber, worldRng, player.day);
  InsuranceDay(player.medical, player.cash, player.owed, player.day);

  // **And the three that are not your fault, or not an event.** Sickness
  // rolls off how you are living -- hungry, run down, and cold in the van
  // is a description of somebody about to get ill, not a die. The tooth
  // only ever goes one way. The prehab streak decays if you stop.
  //
  // Warmth is **the night's own low**, read off the same weather the crag
  // reads: a van is not insulation, and the two things a dirtbag actually
  // controls about a cold night are whether they ate and how wrecked they
  // already were. Freezing is zero, a mild night is one.
  const double lowTonight =
      GenerateWeather(worldRng, player.day).lowTempF;
  const double warmth = Clamp01((lowTonight - 28.0) / 34.0);
  SicknessDay(player.sickness, player.climber, day.hunger, warmth, worldRng,
              player.day);
  TeethDay(player.teeth, worldRng, player.day);
  UpkeepDay(player.upkeep, player.day);
  // What an aching tooth takes, nightly and quietly. It is not a mood
  // until it is an ache -- a twinge is a warning, and warnings are free.
  player.climber.psyche =
      Clamp01(player.climber.psyche - ToothPsycheCost(player.teeth));
  if (player.sickness.active) {
    player.climber.psyche =
        Clamp01(player.climber.psyche - AilmentDials{}.sickPsycheCost);
  }

  // A day older. Nothing is subtracted before the relevant peak, so a
  // twenty-four-year-old is not quietly being taxed from day one.
  AgeDay(player.climber, player.day);

  // And a day nearer a Dirtbag Year, if nobody owns your hours. Counted at
  // the same place as ageing because it is the same kind of fact: something
  // that happens to you for getting through another day rather than
  // something you did.
  DirtbagDay(player.job);

  // And a day of the town watching who gets out of the van. Same reason it
  // lives here: being called something is not an action you take.
  CrewDay(player.crew, player.name, player.bonds, player.standing, worldRng,
          player.day);

  // A day of their career, which is the same night as yours. They chase
  // your grade if they still can; they stop when they are past it; and one
  // season they hang it up and somebody else steps up -- which is the whole
  // point of ageing them, because a rival who ticks up forever is a
  // difficulty slider with a name.
  //
  // Here rather than in a call the engine remembers to make, for the reason
  // the four ticks below it are here: **anything that counts down, counts
  // down at night.** Three per-day rules in this project have been written
  // and left uncalled, and every one was found late.
  if (!player.rival.name.empty()) {
    const double yourGrade =
        SkillToGrade((player.climber.skills.power +
                      player.climber.skills.fingers +
                      player.climber.skills.technique +
                      player.climber.skills.endurance +
                      player.climber.skills.head) / 5.0);
    RivalDay(player.rival, yourGrade, player.day);
    if (!player.rival.retired &&
        ThinkingAboutIt(player.rival, worldRng, player.day)) {
      player.pastRivals.push_back(
          Retire(player.rival, worldRng, player.day));
      player.rival = Succeed(worldRng, player.climber.skills, yourGrade,
                             player.day, player.rival.generation + 1);
    }
  }

  // **The circuit runs itself.** A season opens when there is none, closes
  // when its last comp has been climbed, and the next one starts after the
  // break -- so a career that never enters a comp still has a circuit going
  // on around it, which is the point of a scene that does not wait for you.
  //
  // Here for the reason the four ticks below it are here: anything that
  // counts down, counts down at night.
  if (player.circuit.season <= 0) {
    player.circuit = StartSeason(worldRng, player.day, 1);
  } else if (SeasonOver(player.circuit)) {
    // A break, then the next one. The break is what a close season is.
    const int last = player.circuit.schedule.empty()
                         ? player.day
                         : player.circuit.schedule.back();
    if (player.day - last >= CompDials{}.seasonBreakDays) {
      const int next = player.circuit.season + 1;
      const int titles = player.circuit.titles;
      player.circuit = StartSeason(worldRng, player.day, next);
      player.circuit.titles = titles;
    }
  } else {
    // **A date in the past that nobody resolved is a no-show.**
    //
    // The count is the whole of it, and the first version got it wrong:
    // checking only `compsDone < compsPerSeason` forfeited the day after
    // *every* comp, **including the ones you entered and won**, because a
    // season is five long and one done is still fewer than five. What
    // separates a comp you climbed from one you skipped is not the date --
    // both are in the past -- it is whether the ledger caught up with the
    // calendar.
    const int due = CompsDueBy(player.circuit, player.day - 1);
    while (player.circuit.compsDone < due) {
      Forfeit(player.circuit, player.rankingRecord, player.day);
    }
  }

  // **The ranking is recomputed, never accumulated.** Here, every night,
  // whether or not anything happened -- because the thing that changes it
  // on a quiet night is a result from last year ageing off the end of the
  // window, and nothing else in the game would notice that.
  //
  // This is what makes the named rungs mean something about the climber
  // you are now rather than the best you ever were: stop competing and you
  // come off the team, exactly as you would.
  player.rankingPoints = RankingFrom(player.rankingRecord, player.day);

  // **And the top of the ladder, which runs whether you can see it or
  // not.** A World Cup season opens on the first night of a career, the
  // rounds you did not fly to are banked by the people who did, and the
  // Games are seeded onto their cycle. All of it in one call, here with
  // everything else that counts down at night -- see the note on
  // `WorldStageDay` for why it is one call and not five.
  WorldStageDay(player.worldCup, player.olympics, worldRng, player.day);

  // And the other end of it: the Wednesday night at the gym. Schedules
  // itself, rolls itself forward, and closes its own eight-week block --
  // and **the locals turn up whether you do or not**, so a block you
  // skipped is a block you came last in, which is exactly what happens if
  // you stop going to a real one.
  LeagueDay(player.league, worldRng, player.day);

  // Rent, and a bought year running down. Rent lands here with the other
  // things that happen to you overnight rather than at the shop, because
  // that is what rent does.
  DreamDay(player.dreams, player.cash, player.owed);

  // The month runs down like everything else that runs out. Without this
  // the probe reported 365 days of membership bought with a single $75,
  // which is a very good gym.
  KitDay(player.kit);

  // And so does a closure. This lives here rather than being a call the day
  // loop remembers to make, because it was exactly that and the engine
  // never made it: standing would never have drifted back toward neutral
  // and a shut crag would have stayed shut for the rest of the save. Third
  // per-day tick to be written and left uncalled — anything that counts
  // down now counts down here, where a night is.
  FactionDay(player.standing, worldRng, player.day);

  // A day spent hurt is a day the sponsor is not allowed to hold against
  // you, and the counting of it is a per-night tick like everything else
  // here. Fourth rule-of-thumb application: anything that counts, counts
  // here, where a night is.
  if (IsHurt(player.climber)) player.sponsor.daysHurtThisSeason++;

  player.day += 1;
  // Bills land on their morning, every billsEveryDays-th day after day 1.
  if (dials.billsEveryDays > 0 && player.day > 1 &&
      (player.day - 1) % dials.billsEveryDays == 0) {
    // What living costs *you*. The Desert Local's lane: you know how to
    // live on nothing, and it never stops being true.
    Charge(player, dials.billsAmount * DailyCostMultiplier(player.character) *
                       HabitDailyCost(player.quirks));
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

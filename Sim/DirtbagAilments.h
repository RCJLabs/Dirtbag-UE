#pragma once

// The things that are wrong with you that are not the injury.
//
// `DirtbagMedical` is about one event and its consequences: you hurt
// something, you find out how badly, you decide what to do, and the joint
// carries it. **Every injury in this game is something you did.** That is
// deliberate — an injury must never be weather — and it leaves three
// shapes unbuilt, all of which the 2D game has and all of which are a
// different kind of wrong.
//
// **Sickness is not your fault**, and that is its entire job. It arrives
// because you slept cold and hungry with a body already run down, it takes
// a week, and there is nothing to decide except whether to spend eleven
// dollars making it shorter. It is the counterweight to an injury system
// where everything is earned.
//
// **Teeth only go one way.** A twinge becomes an ache becomes an abscess,
// on a clock, and no amount of resting fixes it — the only thing that ever
// has is money, and it costs more at every stage. Ignoring it is always
// available and always wrong, which makes it the exact inverse of an
// injury, where ignoring it is sometimes fine. It is the game's one pure
// test of whether you will spend on something that is not climbing.
//
// **Prehab is boring, it works, and nobody does it.** Twenty minutes of a
// morning, no money, and it lowers the odds of the thing you cannot see
// coming. The reason to build it is that it makes not doing it a decision.
//
// **The shrink** is the only thing that buys psyche back. Psyche is the
// number a career actually runs on and everything grinds it down; there
// has never been a way to spend on it.
//
// Engine-free like everything in Sim/.

#include <string>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// ---------------------------------------------------------------------
// Sickness
// ---------------------------------------------------------------------

struct Sickness {
  bool active = false;
  int daysLeft = 0;
  double severity = 0.0;   // 0..1
  // Whether you spent the eleven dollars. Shortens it; does not prevent
  // the next one.
  bool medicated = false;
  int caught = 0;          // career
};

// ---------------------------------------------------------------------
// Teeth
// ---------------------------------------------------------------------

// **The staged clock that only escalates.** Nothing here ever improves on
// its own, which is what makes it different from every other body system
// in the game.
enum class ToothStage {
  Fine = 0,
  Twinge,    // you notice it on cold water
  Ache,      // you notice it all the time
  Abscess,   // it is now the main thing about your week
};
constexpr int kToothStageCount = 4;
// unwired-ok: the engine mirrors ToothStage as a UENUM whose DisplayName
// carries the same words, and `TeethText` says the rest in a sentence.
const char* ToothName(ToothStage s);

struct Teeth {
  ToothStage stage = ToothStage::Fine;
  int sinceDay = 0;   // when this stage began
  int fixes = 0;      // times you have paid to make it go away
  int worstEver = 0;  // as an int, so the save does not need an enum
  // **Teeth you did not pay for.** An abscess left alone does eventually
  // stop hurting, and the way it stops is the tooth. Recorded because a
  // career remembers, and because it is the only permanent mark this
  // system leaves.
  int lost = 0;
};

// ---------------------------------------------------------------------
// What you do about it before it happens
// ---------------------------------------------------------------------

struct Upkeep {
  // Prehab: the streak is what counts, not the total. Twenty minutes once
  // is nothing; twenty minutes most mornings is the whole effect.
  int lastPrehabDay = 0;
  int prehabStreak = 0;
  int prehabDays = 0;    // career

  // The shrink.
  int lastShrinkDay = 0;
  int shrinkSessions = 0;
};

struct AilmentDials {
  // --- sickness --------------------------------------------------------
  // **Rolled nightly, and only ever off how you are living.** A fed,
  // rested climber in a warm van essentially never gets ill; a hungry one
  // sleeping cold on a wrecked body does, and neither of those is a dice
  // roll about them, it is a description of them.
  double sickBaseChance = 0.0035;
  // What being run down multiplies it by, at the extreme of each.
  double sickHungerFactor = 2.6;
  double sickLoadFactor = 2.2;
  double sickColdFactor = 2.0;
  // Being ill while ill is not a thing.
  double sickDaysBase = 4.0;
  double sickDaysPerSeverity = 9.0;

  // What it costs while it has you: a flat grade penalty on everything,
  // because you are ill rather than injured, and a ceiling on the energy a
  // night gives back.
  double sickGradePenalty = 1.8;
  double sickEnergyCeiling = 62.0;
  double sickPsycheCost = 0.10;

  // Eleven dollars at the shop. **The cheapest decision in the game**, and
  // it is here to be a decision anyway: a career that will not spend
  // eleven dollars on itself is a career that is telling you something.
  double medsCost = 11.0;
  double medsDaysOff = 0.45;   // fraction of what is left

  // --- teeth -----------------------------------------------------------
  // How long each stage lasts before it becomes the next one. **Long, and
  // it never stops.** A career is ten to thirty years; a tooth left alone
  // reaches an abscess inside two.
  int toothTwingeAfter = 220;
  int toothAcheAfter = 260;
  // **And how long an abscess lasts before it resolves itself**, which it
  // does, the worst possible way: the tooth comes out. That is what bounds
  // the damage of never paying -- it costs you the better part of a year
  // of misery and a tooth, rather than the rest of your career.
  int toothAbscessAfter = 300;
  // And the odds a fresh one starts, per night, once the last is dealt
  // with. Deliberately not weather: it is a slow certainty rather than an
  // event, so this is the rate at which the certainty restarts.
  double toothStartChance = 0.0009;

  // What each stage takes off you daily, in psyche, and what an abscess
  // takes off the day.
  double toothAchePsyche = 0.04;
  double toothAbscessPsyche = 0.10;
  // **Sized down hard after measurement.** At 18 energy and 1.2 grades an
  // abscess was permanent -- it escalated and then never resolved -- so a
  // career that would not pay spent **10,688 of 10,950 days** with one and
  // came out with four sends instead of twenty-four. That is not a money
  // test, it is a silent career-ender wearing a tooth's coat. It has to
  // hurt enough that you pay and not enough that not paying is the end.
  double toothAbscessEnergy = 9.0;
  double toothAbscessGrade = 0.5;

  // **And what it costs to make it go away, which only ever goes up.**
  // A filling is a shift; a root canal is most of a month.
  double toothTwingeCost = 70.0;
  double toothAcheCost = 240.0;
  double toothAbscessCost = 780.0;

  // --- prehab ----------------------------------------------------------
  double prehabHours = 0.35;
  // How many mornings in a row it takes to be worth anything, and what it
  // is worth at full strength: **a third off the odds of a tweak.** Sized
  // to be worth doing and never enough to make you safe.
  int prehabStreakFor = 10;
  double prehabRiskCut = 0.34;
  // Miss more than this many days and the streak is gone. Life happens.
  int prehabGraceDays = 3;

  // --- the shrink ------------------------------------------------------
  // **The only thing in the game that buys psyche.** Priced against a
  // physio, because it is the same trade in a different currency, and rate
  // limited for the same reason.
  double shrinkCost = 110.0;
  int shrinkDaysBetween = 14;
  double shrinkPsyche = 0.22;
  // And it does something a rest day cannot: it raises where psyche drifts
  // back to overnight, for a while.
  double shrinkFloorLift = 0.08;
  int shrinkLasts = 30;
};

// --- sickness -----------------------------------------------------------

// One night of it. Rolls for a fresh one off how you are living, counts an
// existing one down, and clears it when it is over. Returns true on the
// night you come down with something.
bool SicknessDay(Sickness& sick, const Climber& climber, double hunger,
                 double warmth, const Rng& worldRng, int day,
                 const AilmentDials& dials = AilmentDials{});

// Eleven dollars. Takes a chunk off what is left; does not stop the next
// one. False if you are not ill, already medicated, or cannot pay.
bool TakeSomethingForIt(Sickness& sick, double& cash,
                        const AilmentDials& dials = AilmentDials{});

// What being ill costs on the wall, in grade units. Zero when you are not.
double SickPenalty(const Sickness& sick,
                   const AilmentDials& dials = AilmentDials{});

std::string SickText(const Sickness& sick,
                     const AilmentDials& dials = AilmentDials{});

// --- teeth --------------------------------------------------------------

// One night of the clock. **It only ever goes one way.** Returns true on
// the night it gets worse, which is the only news teeth ever have.
bool TeethDay(Teeth& teeth, const Rng& worldRng, int day,
              const AilmentDials& dials = AilmentDials{});

// What it costs to make it go away at the stage it is at now.
double ToothPrice(ToothStage stage,
                  const AilmentDials& dials = AilmentDials{});

// Pay it. Resets to Fine, and the clock can start again another year.
bool FixTheTooth(Teeth& teeth, double& cash, int day,
                 const AilmentDials& dials = AilmentDials{});

// What it takes off you: psyche nightly, and at the far end the day itself.
double ToothPsycheCost(const Teeth& teeth,
                       const AilmentDials& dials = AilmentDials{});
double ToothGradePenalty(const Teeth& teeth,
                         const AilmentDials& dials = AilmentDials{});

std::string TeethText(const Teeth& teeth,
                      const AilmentDials& dials = AilmentDials{});

// --- prehab -------------------------------------------------------------

// Twenty minutes of a morning. Costs hours and nothing else. False if you
// have already done it today.
bool DoPrehab(Upkeep& upkeep, double& hour, int day,
              const AilmentDials& dials = AilmentDials{});

// The streak decays if you stop. Called nightly.
void UpkeepDay(Upkeep& upkeep, int day,
               const AilmentDials& dials = AilmentDials{});

// What the streak is worth as a multiplier on injury risk. One when you
// have not been doing it -- **never below the cut**, so prehab lowers the
// odds and never removes them.
double PrehabRisk(const Upkeep& upkeep,
                  const AilmentDials& dials = AilmentDials{});

// --- the shrink ---------------------------------------------------------

// An hour of talking about it. Returns false if you cannot pay or it is
// not due.
bool SeeTheShrink(Upkeep& upkeep, Climber& climber, double& cash, int day,
                  const AilmentDials& dials = AilmentDials{});

// Where psyche drifts back to overnight, which the shrink lifts for a
// while. The day loop's baseline is the floor under this.
double PsycheFloor(const Upkeep& upkeep, double baseline, int day,
                   const AilmentDials& dials = AilmentDials{});

std::string UpkeepText(const Upkeep& upkeep, int day,
                       const AilmentDials& dials = AilmentDials{});

}  // namespace dirtbag

// The day loop — the 2D game's gym-day rhythm as plain state and actions.
// Wake in the van, get to the gym, burn attempts, eat, work, sleep; skills
// creep, skin regrows, bills land. The session layer (DirtbagSessionLoop)
// handles what happens on the wall; this layer handles everything around it.
// Engine-free like all of Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"
#include "DirtbagSessionLoop.h"

namespace dirtbag {

// Every number a designer might turn, with its reason. Values seeded from
// the 2D game's balance instincts; the 7-day integration test is the canary
// when they move.
struct DayDials {
  double wakeHour = 7.0;

  // Hunger 0..100 climbs through the day; two meals keeps it civilized.
  // Sleeping hungry ruins the night's recovery — the dirtbag's oldest trap.
  double hungerPerHour = 3.0;
  double mealCost = 8.0;        // gas-station burrito economics
  double mealHunger = 45.0;     // one meal buys back this much
  double starvingHunger = 70.0; // above this, sleep recovers poorly

  // Work: the classic four-hour belay-desk shift.
  double shiftHours = 4.0;
  double shiftWage = 60.0;
  double shiftEnergy = 30.0;

  // A burn costs time and body even before the wall takes its share.
  double attemptHours = 0.25;
  double attemptEnergy = 4.0;

  // Sleep: full recovery fed and rested; a hungry night bottoms out here.
  double sleepEnergyFloor = 55.0;

  // Skin grows back slowly — about six nights from wrecked to fresh.
  double skinRegenPerNight = 1.5;
  double maxSkin = 9.0;

  // The money clock: rent-adjacent bills land once a week, small but
  // relentless. This is the pressure that makes shifts non-optional.
  int billsEveryDays = 7;
  double billsAmount = 85.0;

  // Training: attempts near or above your level move the needle; laps on
  // jugs two grades below you move nothing. ~0.1/attempt means a committed
  // week visibly creeps a skill — the 2D game's glacier pace.
  double trainingRate = 0.12;

  // Running on empty shows up as nerve before it shows up as strength.
  double fatigueEnergy = 35.0;
  double fatiguePsyche = 0.15;

  // Overnight, psyche drifts a quarter of the way back to the 0.7 baseline
  // — yesterday's heartbreak fades, it doesn't vanish.
  double psycheBaseline = 0.7;
  double psycheHomeRate = 0.25;
};

// Career state — everything that outlives a day. This is what the save file
// carries (DirtbagSave.h).
struct PlayerState {
  Climber climber;
  double cash = 420.0;  // the war chest you left home with
  int day = 1;
  std::vector<ProjectMemory> projects;
};

// One day's body-clock. Created at wake, consumed by sleep, never saved —
// saves happen at day boundaries.
struct DayState {
  double hour = 7.0;
  double energy = 100.0;
  double hunger = 0.0;
  bool atGym = false;
  SessionState session;  // meaningful once StartGymSession has run
};

DayState WakeUp(const PlayerState& player, const DayDials& dials = DayDials{});

// Time passing is never free: hunger rides along.
void PassHours(DayState& day, double hours, const DayDials& dials = DayDials{});

// Returns false when the wallet says no.
bool EatMeal(PlayerState& player, DayState& day, const DayDials& dials = DayDials{});

void WorkShift(PlayerState& player, DayState& day, const DayDials& dials = DayDials{});

// The climber as they are right now: career skills, current skin, and
// today's fatigue speaking through psyche.
Climber ClimberForSession(const PlayerState& player, const DayState& day,
                          const DayDials& dials = DayDials{});

// Pull on: seeds the day's SessionState from the current climber.
void StartGymSession(PlayerState& player, DayState& day,
                     const DayDials& dials = DayDials{});

// The project ledger for a route, created on first touch.
ProjectMemory& MemoryFor(PlayerState& player, const Route& route);

// Book one burn's day-costs (time, energy) and its training creep. Call
// after the attempt resolves (AttemptInSession or a committed live attempt);
// the session/ledger themselves are already paid by the session layer.
void ApplyAttemptToDay(PlayerState& player, DayState& day, const Route& route,
                       const AttemptResult& result,
                       const DayDials& dials = DayDials{});

// Lights out: skin regrows, psyche drifts home, the day advances, bills
// land on their morning. The session's remaining skin becomes tomorrow's.
void SleepToNextDay(PlayerState& player, DayState& day,
                    const DayDials& dials = DayDials{});

// The gym's route board: `count` routes laddered V0 upward, deterministic
// per seed, with the occasional in-house sandbag (setters are people too).
std::vector<Route> GymBoard(const Rng& worldRng, int count = 8);

}  // namespace dirtbag

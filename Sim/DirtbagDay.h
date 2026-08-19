// The day loop — the 2D game's gym-day rhythm as plain state and actions.
// Wake in the van, get to the gym, burn attempts, eat, work, sleep; skills
// creep, skin regrows, bills land. The session layer (DirtbagSessionLoop)
// handles what happens on the wall; this layer handles everything around it.
// Engine-free like all of Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagDog.h"
#include "DirtbagGear.h"
#include "DirtbagVan.h"
#include "DirtbagPartner.h"
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

  // A burn costs time and body even before the wall takes its share, and
  // trying hard costs more than cruising: energy per grade the line is
  // above you, so a limit session drains where a mileage day doesn't.
  double attemptHours = 0.25;
  double attemptEnergy = 4.0;
  double attemptEnergyPerGrade = 2.5;

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
  // jugs two grades below you move nothing. Calibrated so a committed week
  // (≈28 burns at or above your limit) is worth roughly a third of a grade
  // at mid-career — visible progress, glacial mastery, the 2D game's pace.
  // One grade is ~7 skill points under the resolver's skillGradeSpan.
  double trainingRate = 0.07;

  // Skill approaches this asymptote, never reaches it: at skill 45 a session
  // trains at ~0.64 rate, at 80 ~0.36, and the last few points cost years.
  double trainingCeiling = 125.0;

  // Mileage's share, per move climbed — deliberately below the targeted
  // skills so volume supports a career instead of replacing it.
  double enduranceMileageRate = 0.06;

  // How much of the line you actually climbed, as a multiplier on what it
  // taught you. Without this, training reads only the grade on the tag and
  // a season of falling off the first move of something impossible trains
  // exactly as well as a season of nearly doing it — measured, a policy
  // that always picked the hardest line went V5 to V8.7 in a year without
  // sending anything at all. You get strong by doing hard moves, not by
  // touching the start holds of a line that is not yours yet.
  //
  // The floor is what pulling on teaches regardless: real, small, and far
  // below what linking most of a line is worth, so the best thing to train
  // on stays the thing you can nearly do.
  double engagementFloor = 0.2;

  // Running on empty shows up as nerve before it shows up as strength, and
  // it arrives gradually: above freshEnergy you're fine, below it the fade
  // ramps in, reaching fatiguePsyche at zero. A cliff-edge threshold was
  // unreachable in practice — a session ends on skin long before energy —
  // so the tax has to start while you're still climbing.
  double freshEnergy = 75.0;
  double fatiguePsyche = 0.3;

  // What an hour of sitting buys back. Small on purpose: resting is how you
  // spend time you cannot climb in, not a way to farm energy — at 4 an hour
  // a whole afternoon in the shade is worth less than a night's sleep.
  double restEnergyPerHour = 4.0;

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

  // What you could not pay. Bills land whether or not the money is there;
  // cash floors at nothing and the shortfall goes here, because a bill you
  // cannot pay does not evaporate — it waits, and it is the first thing any
  // wage goes to. Owing money is not a lock on anything: you can still
  // climb, eat, and drive. You simply cannot get ahead until it is cleared,
  // which is the whole of what being behind feels like.
  double owed = 0.0;

  std::vector<ProjectMemory> projects;

  // Who you know at the Lot, and what they got to first. Strength is
  // derived from the seed and the date, so only this much is career state.
  std::vector<PartnerBond> bonds;

  // The stray, and then the dog.
  Dog dog;

  // What is on your feet, and how much of it is left.
  Shoes shoes;

  // The van: shelter, transport, and the reason seasons end early.
  Van van;
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

// Take money, and remember what could not be taken. Cash never goes
// negative; the shortfall becomes debt.
void Charge(PlayerState& player, double amount);

// Give money, debt first. Nothing reaches your pocket until you are level,
// which is what a wage feels like when you are behind.
void Pay(PlayerState& player, double amount);

DayState WakeUp(const PlayerState& player, const DayDials& dials = DayDials{});

// Time passing is never free: hunger rides along.
void PassHours(DayState& day, double hours, const DayDials& dials = DayDials{});

// Sitting it out — in the shade, at the fire, waiting for the rock to come
// into condition. The hours pass and hunger rides along exactly as it would
// anyway; what you get back is a little energy, nothing like a night's
// worth. Waiting is a real move at a crag and needs to cost time without
// being punished for it.
void Rest(DayState& day, double hours, const DayDials& dials = DayDials{});

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

// --- The career, read back ---------------------------------------------------

// What the ledgers add up to. Derived, never stored: the project ledgers are
// the truth, this is only how you'd say it out loud.
struct CareerSummary {
  double abilityGrade = 0.0;   // what the sim says you are
  int hardestSendGrade = -1;   // what you have actually done; -1 = nothing yet
  std::string hardestSendName;
  Style hardestSendStyle = Style::Fell;
  int totalSends = 0;
  int totalAttempts = 0;
  int openProjects = 0;        // touched, not sent
  std::string nemesis;         // the unsent line you have fed the most burns
  int nemesisAttempts = 0;
};

CareerSummary SummarizeCareer(const PlayerState& player);

// The summary in the game's own voice — one dry line for a stats card.
std::string CareerLine(const CareerSummary& career);

}  // namespace dirtbag

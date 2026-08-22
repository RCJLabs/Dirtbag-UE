// The day loop — the 2D game's gym-day rhythm as plain state and actions.
// Wake in the van, get to the gym, burn attempts, eat, work, sleep; skills
// creep, skin regrows, bills land. The session layer (DirtbagSessionLoop)
// handles what happens on the wall; this layer handles everything around it.
// Engine-free like all of Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagAge.h"
#include "DirtbagCore.h"
#include "DirtbagCrew.h"
#include "DirtbagDreams.h"
#include "DirtbagDog.h"
#include "DirtbagFactions.h"
#include "DirtbagGear.h"
#include "DirtbagJobs.h"
#include "DirtbagKit.h"
#include "DirtbagEthics.h"
#include "DirtbagSponsor.h"
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

  // What head trains on: the boldest move you actually committed to, in the
  // grade units ExposureAt already prices. Head is the one skill that does
  // not train on trying hard — a scary move on an easy line teaches it and a
  // desperate move on a well-padded one does not — which is why it is
  // deliberately outside the challenge/engagement gate the other four sit
  // behind.
  //
  // It cannot be farmed. Exposure is zero low down, zero under pads, and
  // zero below the first bolt's worth of rope, so the only way to earn it is
  // to be high on something you could get hurt on. Buying a second pad
  // genuinely trades head for sends, which is the trade a real boulderer
  // makes and the first place in this game where kit costs you something.
  //
  // Sized by measurement, not by feel: at 0.20 a season on one pad is
  // worth about a point and a half of head, a season on bare ground about
  // three, and a season behind two pads almost nothing.
  double headExposureRate = 0.20;

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

  // The last day you saw a physio. Physio is the one thing money buys that
  // hands you climbing back rather than moving it around, and it is rate
  // limited so a rich season cannot buy its way out of a bad one overnight.
  int lastPhysioDay = 0;

  // The things you did that nobody saw. Carried, not priced — an act only
  // costs anything on the day somebody finds out.
  std::vector<Secret> secrets;

  // Who pays you to climb, and what they want for it. The only money in
  // the game that arrives *because* you climbed rather than instead of it.
  Sponsorship sponsor;

  // What you own that buys you climbing: pads, a board, the gym. The
  // answer to the measurement that said the year ended with $133 in the
  // bank and nowhere for it to go.
  Kit kit;

  // The van: shelter, transport, and the reason seasons end early.
  Van van;

  // Work, and whether it owns you.
  Job job;

  // What the town calls the people you keep turning up with. Not yours to
  // choose and not yours to change -- see DirtbagCrew.h.
  Crew crew;

  // What the money is for. See DirtbagDreams.h -- the price is the buffer,
  // not the number.
  Dreams dreams;

  // Where you stand with the scene, and whether the crag is still open.
  Standing standing;
};

// One day's body-clock. Created at wake, consumed by sleep, never saved —
// saves happen at day boundaries.
struct DayState {
  double hour = 7.0;
  double energy = 100.0;
  double hunger = 0.0;
  bool atGym = false;
  // One session a day. The board is not a slot machine, and without this
  // the only thing stopping you was skin — which bought eleven hangs and
  // a day that trained more than the wall ever could.
  bool hangboardDone = false;
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

// Take a gig off the board: its hours, its energy, its money. Returns false
// if it needs the van and the van is not going anywhere — which is how a
// breakdown costs you the fix and the work that would have paid for it.
bool WorkOddJob(PlayerState& player, DayState& day, const OddJob& job,
                const DayDials& dials = DayDials{});

// Do the day the salary owns. Pays a fifth of the week, takes the middle of
// it, and the middle of the day is when the rock is in condition.
void WorkSalariedDay(PlayerState& player, DayState& day,
                     const JobDials& jobs = JobDials{},
                     const DayDials& dials = DayDials{});

// Sign on, and walk out. Neither is punished; the hours do that.
void TakeSalariedJob(PlayerState& player);
void QuitSalariedJob(PlayerState& player, const JobDials& jobs = JobDials{});

// The climber as they are right now: career skills, current skin, and
// today's fatigue speaking through psyche.
Climber ClimberForSession(const PlayerState& player, const DayState& day,
                          const DayDials& dials = DayDials{});

// Pull on: seeds the day's SessionState from the current climber.
// `kit` is a parameter rather than a `KitDials{}` built inside, because
// what the pads under you are worth is a tunable and it was not reachable
// from any caller.
void StartGymSession(PlayerState& player, DayState& day,
                     const KitDials& kit = KitDials{},
                     const DayDials& dials = DayDials{});

// A day on plastic. The only climbing that ignores the weather, and the
// whole argument for $75 a month: a season has 157 days that never come
// good and 75 more spent resting skin, and without a membership every one
// of them is dead time no amount of money can touch.
//
// False if you are not a member — the gym is the one place in this game
// that checks.
bool GoToTheGym(PlayerState& player, DayState& day,
                const KitDials& kit = KitDials{},
                const DayDials& dials = DayDials{});

// An hour on the board bolted above the van door. The broke answer to a
// washed-out day: it trains fingers and nothing else, it costs skin, and it
// teaches you nothing about movement. False if you do not own one, or if
// there is not enough skin left to be worth hanging on.
bool HangboardSession(PlayerState& player, DayState& day,
                      const KitDials& kit = KitDials{},
                      const DayDials& dials = DayDials{});

// The project ledger for a route, created on first touch.
ProjectMemory& MemoryFor(PlayerState& player, const Route& route);

// Book one burn's day-costs (time, energy) and its training creep. Call
// after the attempt resolves (AttemptInSession or a committed live attempt);
// the session/ledger themselves are already paid by the session layer.
// `worldRng` is required for the same reason SleepToNextDay's is: booking a
// burn's costs is where pulling on an injury either gets away with it or
// makes it worse, and a caller who left that out would play a game where
// climbing hurt never cost anything.
void ApplyAttemptToDay(PlayerState& player, DayState& day, const Route& route,
                       const AttemptResult& result, const Rng& worldRng,
                       const DayDials& dials = DayDials{});

// Lights out: skin regrows, psyche drifts home, the day advances, bills
// land on their morning. The session's remaining skin becomes tomorrow's.
// `worldRng` is required rather than defaulted because this is where the
// body rolls: load comes down and, above the threshold, an injury lands.
// A caller who forgot it would silently play a game where nobody ever gets
// hurt, and with no Unreal compiler in the loop that is the exact class of
// mistake that reaches the PC. Making it a parameter makes it impossible.
void SleepToNextDay(PlayerState& player, DayState& day, const Rng& worldRng,
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

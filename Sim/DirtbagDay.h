// The day loop — the 2D game's gym-day rhythm as plain state and actions.
// Wake in the van, get to the gym, burn attempts, eat, work, sleep; skills
// creep, skin regrows, bills land. The session layer (DirtbagSessionLoop)
// handles what happens on the wall; this layer handles everything around it.
// Engine-free like all of Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagAge.h"
#include "DirtbagCharacter.h"
#include "DirtbagHabits.h"
#include "DirtbagComp.h"
#include "DirtbagTeam.h"
#include "DirtbagLeague.h"
#include "DirtbagMedical.h"
#include "DirtbagAilments.h"
#include "DirtbagConditions.h"
#include "DirtbagWorldStage.h"
#include "DirtbagCore.h"
#include "DirtbagMonotony.h"
#include "DirtbagRival.h"
#include "DirtbagStyle.h"
#include "DirtbagTax.h"
#include "DirtbagCrew.h"
#include "DirtbagDreams.h"
#include "DirtbagDog.h"
#include "DirtbagFactions.h"
#include "DirtbagGear.h"
#include "DirtbagCraft.h"
#include "DirtbagJobs.h"
#include "DirtbagKit.h"
#include "DirtbagLife.h"
#include "DirtbagGym.h"
#include "DirtbagGymFloor.h"
#include "DirtbagGymLeague.h"
#include "DirtbagYouth.h"
#include "DirtbagBivy.h"
#include "DirtbagLiving.h"
#include "DirtbagLocals.h"
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

  // --- What the logbook counts ------------------------------------------
  // Three thresholds, here rather than in HabitDials because all three are
  // facts about *a day in this game* rather than about habits: what hour
  // counts as early is a property of the light, what counts as thin skin is
  // the same number the session advice uses, and what counts as grinding is
  // a fact about how long a project takes here. HabitDials owns the shapes;
  // these own the day.
  double dawnBefore = 8.0;      // pulled on before the sun got to the rock
  int grindingAfter = 6;        // burns on one line before it is a grind
  //
  // What counts as thin skin is deliberately *not* here. SessionLoopDials
  // already owns that number -- it is what the session advice reads to say
  // "your tips are gone; jugs or go home" -- and a second copy would have
  // been a silent mirror. The checker caught it within a minute of it
  // being written, which is the checker doing exactly its job.

  // Hunger 0..100 climbs through the day; two meals keeps it civilized.
  // Sleeping hungry ruins the night's recovery — the dirtbag's oldest trap.
  double hungerPerHour = 3.0;
  double mealCost = 8.0;        // gas-station burrito economics
  double mealHunger = 45.0;     // one meal buys back this much
  double starvingHunger = 70.0; // above this, sleep recovers poorly

  // **When the day ends, whatever you did with it.**
  //
  // Hunger only ever accrued through `PassHours`, so it only counted the
  // hours you *spent* -- and a day you spent nothing on was a day you did
  // not get hungry. Sleep at ten in the morning and the night cost you
  // nothing, which is how a measured thirty-year career came to eat **595
  // meals**, one every eighteen days, and be a stranger in its own town.
  //
  // The engine does exactly the same thing, so this was never a probe
  // artefact: a player who walks to the van and presses sleep skips the
  // day at the same discount. The hours between now and bedtime happen to
  // you whether or not you find something to do with them, and
  // `SleepToNextDay` is the one place that is true of both consumers.
  //
  // Ten at night. Not "last light", which would make a winter cheaper to
  // live through than a summer -- you go to bed when you go to bed.
  double bedtimeHour = 22.0;

  // **How much of last night's hunger is still there in the morning.**
  //
  // Without this, `starvingHunger` is unreachable by construction and
  // always has been: hunger reset to nothing at `WakeUp`, and fifteen
  // waking hours at three an hour is forty-five, so seventy could not
  // happen inside one day however the day went. The probe agrees -- **zero
  // hungry nights across thirty years**, in every career ever measured.
  // *"Sleeping hungry ruins the night's recovery -- the dirtbag's oldest
  // trap"* has been in this file since Phase 1 and has never once fired.
  //
  // At 0.6 the arithmetic works out to the trap it was meant to be: eat
  // once and you are level, skip a day and you wake at 27 and go to bed at
  // 72, which is the first bad night, and the day after that is worse. Two
  // days without food is trouble and one is not, which is about right.
  double hungerKeptOvernight = 0.6;

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

  // `DEPTH-19`: lifestyle creep. **Climbing harder costs more to sustain**
  // -- more road, more food, more wear on the rig -- so the weekly nut
  // rises with the grade you climb, capped at three times.
  //
  // **Half of DEPTH-19 was already here under another name.** Its other
  // clause is that harder lines eat rubber faster, and
  // `GearDials::wearPerGradeOverFive` has done exactly that since Phase 3.
  // This is the half that was missing, and it is the half with teeth: the
  // source's own note says income scales faster, *"so it's a creep not a
  // crush -- but the dirtbag squeeze never fully resolves."*
  double lifestylePerGrade = 0.08;
  double lifestyleCap = 3.0;

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
  // What you are called. The crew hash includes it, so the town names
  // *this* generation's crew rather than re-issuing the last one's; the
  // legacy is where it ends up. Empty is legal and simply adds nothing.
  std::string name;

  Climber climber;

  // Who this climber is, as opposed to what they can do. Chosen at
  // creation, and the origin's money is what `cash` starts as -- see
  // `Sim/DirtbagCharacter.h`. Default-constructed it is an All-Rounder who
  // Sold It All with Gumby and the Lifer's temperament, which is a real
  // person rather than a blank, so every existing caller keeps working and
  // keeps meaning something.
  Character character;

  // Somebody to beat, and the ones who came before. Rolled at the start of
  // a career and replaced when they retire -- see Sim/DirtbagRival.h, and
  // note that this is a *person* and `nemesis` above is a *route*. They are
  // different things and §4 conflated them.
  Rival rival;
  std::vector<PastRival> pastRivals;

  // National ranking points. What the comp tiers gate on, and the number
  // the whole ladder above a gym comp reads -- the team, the World Cup and
  // the Games all key off it. Earned at comps and nowhere else, which is
  // what makes a comp worth entering when the prize money deliberately is
  // not.
  //
  // **Derived, never accumulated.** It is the sum of `rankingRecord` inside
  // the last year, recomputed every night by `SleepToNextDay`, and writing
  // to it directly is a bug: the next morning would overwrite it. It is
  // kept as a field rather than a call because six systems read it and a
  // rolling sum recomputed six times a day is worse than one recomputed
  // once a night.
  double rankingPoints = 0.0;

  // What the number above is made of. **A ranking is a record of the last
  // year, not a lifetime total** -- see Sim/DirtbagComp.h. Pruned as it is
  // written, so this stays about twenty-five entries long however long the
  // career runs.
  std::vector<RankingResult> rankingRecord;

  // The season you are in the middle of. Five firm dates, the last worth
  // half as much again -- see Sim/DirtbagComp.h. A season with `season == 0`
  // has not started, which is where a career begins and where an old save
  // lands.
  Circuit circuit;

  // Whether your name is on the paper. Reviewed when a circuit season
  // closes and at no other time -- a domestic season is the unit a
  // selection committee actually works in. See Sim/DirtbagTeam.h.
  NationalTeam team;

  // The top of the ladder. A World Cup season is running from the first
  // night of a career whether or not you have ever heard of it -- **the
  // field flies whether you do or not**, and a table that only starts
  // moving once you are good enough to see it is a table that waited for
  // you. See Sim/DirtbagWorldStage.h.
  WorldCupSeason worldCup;

  // And the Games, on a cycle rather than a calendar. Seeded on the first
  // night, and never sooner than three weeks out.
  Olympics olympics;

  // `SPEED-2`: the fastest clean lap you have ever run on a speed wall, in
  // seconds. **Zero means never, not instant** -- see PersonalBestLine,
  // which refuses to print a wall record nobody has set.
  double speedPersonalBest = 0.0;

  // `TAX-1`: the year's prize money and when it was last settled. Only
  // prize money -- shifts are cash and off the books, which is the whole
  // point of the life. See Sim/DirtbagTax.h.
  Tax tax;

  // `DEPTH-6`: every attempt and every send, by style. What a career of
  // climbing made you, and the one thing here that is not decayed -- a
  // career does not forget how to crimp over a quiet winter. See
  // Sim/DirtbagStyle.h.
  StyleLog style;

  // `CHAR-6` / `CHAR-6b`: the moves the scene named after you. Empty names
  // until they are earned, and the second is always a different style.
  Signature signature;
  Signature signature2;

  // `CHAR-7`: per-skill monotony. The reason the same session every day
  // stops paying, which the training ceiling was never able to say -- the
  // ceiling says *you cannot get much stronger*, this says *not like this*.
  Monotony monotony;

  // **What the last week actually cost**, and the day it landed.
  //
  // Bills have been charged since Phase 1 and **nothing has ever said what
  // they were.** The dial is $85; what leaves your pocket is that through
  // the Desert Local's discount, whatever your habits cost you, and now
  // `DEPTH-19`'s creep -- three multipliers, none of which the player or
  // the probe could see. The season probe printed
  // `DAYS / billsEveryDays * billsAmount` and called it a measurement,
  // which is how "bills $132,940" survived three multipliers without ever
  // moving.
  //
  // Recorded like the tax bill and for the same reason: the night tick
  // charges it and Sleep plumbs no return value through.
  double lastBill = 0.0;
  int lastBillDay = -1;

  // `CLB-32`: you have watched the film for the comp that is coming. Moves
  // onto the board when the board is set -- see TakeTheScoutingIn.
  bool scoutedTheField = false;

  // `PSY-2`: how long you have been climbing the same place. Dulls what the
  // climbing gives you and nothing else -- hobbies, the fire and people are
  // the cure. See Sim/DirtbagMonotony.h.
  VenueStaleness stale;

  // The other end of the same system: a Wednesday at the gym, five
  // dollars, and a number that is yours. Worth no ranking points at all --
  // see Sim/DirtbagLeague.h for why that is the design rather than an
  // omission.
  League league;

  double cash = 420.0;  // the war chest you left home with
  int day = 1;

  // **What you went to bed with.** On the career rather than the day
  // because a `DayState` does not survive the night -- every caller builds
  // tomorrow with `WakeUp`, which is exactly where a hunger carried on the
  // day would have been silently thrown away. See
  // `DayDials::hungerKeptOvernight`.
  double hungerCarried = 0.0;

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

  // What is wrong with you, what you know about it, what you did, and what
  // it left behind. **`Climber::injury` is what a body *is*; this is what
  // happened to it** -- see Sim/DirtbagMedical.h.
  Medical medical;

  // And the things that are wrong with you that are not the injury: a
  // cold you caught because you slept hungry and cold, a tooth that only
  // gets worse, twenty minutes of a morning, and somebody to talk to. See
  // Sim/DirtbagAilments.h -- **every injury in this game is something you
  // did, and these are deliberately not.**
  Sickness sickness;
  Teeth teeth;
  Upkeep upkeep;

  // The things you did that nobody saw. Carried, not priced — an act only
  // costs anything on the day somebody finds out.
  std::vector<Secret> secrets;

  // Who pays you to climb, and what they want for it. The only money in
  // the game that arrives *because* you climbed rather than instead of it.
  Sponsorship sponsor;

  // **How you have been climbing, and what it made you.** A tally nothing
  // else in the game keeps: the session resolver knows what you did on one
  // route and the ledger knows what you did on one line, and neither of
  // them can answer *what sort of climber is this*. See Sim/DirtbagHabits.h.
  Logbook logbook;
  Quirks quirks;

  // **What you became last night**, or None, which is almost every night.
  // Carried on the career rather than returned from `SleepToNextDay`
  // because that function is void and has four callers, and a line the
  // player is supposed to read once must not depend on which of them
  // remembered to look. Cleared at the top of every night.
  Quirk becameToday = Quirk::None;

  // **The rack**, which is the only thing you can own that unlocks a whole
  // discipline rather than improving one — no rack, no trad lead, and the
  // rope stays in the van for a different reason than when nobody will
  // belay you. Empty for a career that has never bought one, which is
  // every career until it is not. See Sim/DirtbagTrad.h.
  Rack rack;

  // What you own that buys you climbing: pads, a board, the gym. The
  // answer to the measurement that said the year ended with $133 in the
  // bank and nowhere for it to go.
  Kit kit;

  // The van: shelter, transport, and the reason seasons end early.
  Van van;

  // Work, and whether it owns you.
  Job job;

  // **And what you are getting good at while it does.** A lever you pull
  // for money is a lever; a trade you are getting better at is a life --
  // see Sim/DirtbagCraft.h.
  Craftsman hand;

  // What the town calls the people you keep turning up with. Not yours to
  // choose and not yours to change -- see DirtbagCrew.h.
  Crew crew;

  // What the money is for. See DirtbagDreams.h -- the price is the buffer,
  // not the number.
  Dreams dreams;

  // **Everything that is not climbing.** Somebody, home, the guitar, the
  // books, the stove -- and the fact that all five go cold if you never
  // give them a day. See Sim/DirtbagLife.h; the one that leaves is the one
  // that makes the rest a system rather than a menu.
  Life life;

  // **Where you are parking tonight**, and what the city thinks of it.
  // See Sim/DirtbagBivy.h -- the one decision somebody living like this
  // makes every single day.
  Bivy bivy;

  // **How you are living**, which is three meters and one rule: grime
  // multiplies social gains and touches nothing else. See
  // Sim/DirtbagLiving.h.
  Living living;

  // **The gym, if you bought one.** The one purchase above the dreams'
  // range and the only thing in this game that pays you back -- see
  // Sim/DirtbagGym.h, and `concepts/DECISION-gym-ownership.md` for why a
  // recorded cut came back.
  Gym gym;

  // **And the people in it.** Kept beside the gym rather than inside it,
  // the same way the source keeps it on the state rather than on `ownGym`:
  // these are people in a town, not fixtures in a building, and if the bank
  // takes the lease Dale does not stop existing.
  GymFloor floor;

  // **The youth team, if you founded one.** `GYM-2` wrote the hook -- "There
  // is no youth team. There might have to be" -- and this is what is behind
  // it. See Sim/DirtbagYouth.h.
  Youth youth;

  // **The league you run**, if you started one. Not the one in
  // `DirtbagLeague.h`, which is the one you enter at somebody else's gym --
  // see Sim/DirtbagGymLeague.h for why they are two systems.
  GymLeague gymLeague;

  // **What the bank did last night**, or empty. Carried on the career for
  // the same reason `becameToday` and `lostToday` are: the night tick is
  // void, and losing a building is a thing you must be told exactly once.
  std::string gymNews;

  // **The people behind the counters, and what they are holding.** Seeded
  // on the first night of a career rather than at creation, so an old save
  // walks into a town that has faces in it -- see Sim/DirtbagLocals.h.
  Locals locals;

  // **What you lost last night**, or None, which is nearly every night.
  // Carried on the career for the same reason `becameToday` is: the night
  // tick is void, it has four callers, and a line the player is meant to
  // read once must not depend on which of them thought to ask.
  Thread lostToday = Thread::None;

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

  // **Was it plastic.** `atGym` is misnamed and has been since Phase 1: it
  // means *a session was started today*, and both the wall and the crag set
  // it. Nothing needed the distinction until the logbook did, and inferring
  // it from padding would have been a guess.
  bool indoors = false;

  // When you actually pulled on, or -1 if you never did. The dawn patrol is
  // a habit about the clock and there was nothing anywhere that remembered
  // what time the first burn happened.
  double firstPullOnHour = -1.0;

  // `CHAR-7`: what monotony is worth today, per lane, and whether it has
  // been asked yet.
  //
  // **Stepped once per session, read once per burn.** The source steps its
  // monotony per *session* and the first cut of this port stepped it per
  // burn -- which with eight burns to a session saturates a lane in under
  // two days and pins every session after the first at half gains. Caught
  // by the probe: a thirty-year career reported "nothing stuck" because the
  // level was maxed and then shed on the rest days, and the number that
  // mattered was never in the output at all.
  bool monotonyFed = false;
  double monotonyGain[kSkillCount] = {1.0, 1.0, 1.0, 1.0, 1.0};

  // **Where you climbed today**, for `PSY-2`. Empty means the crag, which
  // is what a sim-only caller with no world around it is doing. Set before
  // the session starts -- `GoToTheGym` sets it, and so does the engine when
  // it knows which rock you are standing at.
  std::string venue;

  // **What somebody said to you today**, or empty, which is most days.
  // On the day rather than in the save for the same reason `lastBurn` is:
  // it is a thing you read once and then get on with the day. The memory
  // behind it is spent the moment it is said -- see Sim/DirtbagLocals.h.
  std::string heard;

  // **What the last burn was**, in one sentence -- see Sim/DirtbagNarrator.h.
  // On the day rather than in the save because it is a thing you read once
  // and then climb again; a career's history is the project ledger's job.
  std::string lastBurn;

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
// **`life` is not defaulted**, and that is deliberate: an hour with a
// paperback rests you better than an hour going over the beta, and a
// default would let a second caller quietly rest at the plain rate forever
// -- the exact shape of every "two paths, one assembling it by hand" bug
// this project has found.
void Rest(DayState& day, double hours, const Life& life,
          const DayDials& dials = DayDials{});

// Returns false when the wallet says no.
bool EatMeal(PlayerState& player, DayState& day, const DayDials& dials = DayDials{});

void WorkShift(PlayerState& player, DayState& day, const DayDials& dials = DayDials{});

// **An evening on something that is not climbing.** The hours pass, hunger
// rides along exactly as it would anyway, and the thread gets warmer. The
// guitar pays while it does -- badly at first, then less badly -- which
// makes it the only work in the game that is also a hobby and the only
// hobby that is also work.
//
// **How long it takes is not the caller's to decide** -- `AsksFor` owns it,
// because one go at a thing being two hours here and six hours there is
// how the evening quietly stopped costing anything. Seeing somebody takes
// the day; a phone call takes half an hour; and that difference is the
// whole reason only one of the five can leave you.
//
// **Takes it up if it is not yours yet, and pays the same hours for it.**
// Measured: with taking-up free, a career that never gave anybody a minute
// still had somebody 60% of the time -- meet, coast on full warmth for
// twenty-six days, lose them at zero depth, sit out the cooldown, repeat,
// about a hundred and ninety times in thirty years. A first date costs a
// day like every other day does.
//
// Returns false when the thread is not in your life and cannot be taken up
// today -- which is the same answer as "you do not own a guitar" and as
// "not this soon after the last one".
bool SpendTheEvening(PlayerState& player, DayState& day, Thread what,
                     const DayDials& dials = DayDials{});

// **Walk the floor of the gym you bought.** An hour among your members --
// it advances whoever is next due a moment, or greets the cohort your set
// mix collected, or tells you what one of them is doing today.
//
// The reason it exists at all is that the P&L engine gives you no reason
// ever to be in the building: it ticks whether you are there or not, and
// once you hand over the keys it runs *better* without you. This is the
// half of ownership that is not a spreadsheet.
//
// Once a day. Returns false if you already have, if there is no gym, or if
// there is not an hour left in the day.
bool WalkTheGymFloor(PlayerState& player, DayState& day,
                     const DayDials& dials = DayDials{});

// **GYM-10: start a league at your own gym.** The night and the format are
// the whole decision -- the format is not cosmetic, it decides who wins,
// and every one of your regulars leans one way. $220 of tape, prizes and a
// printed table, and twenty-five members before it is a league rather than
// four people and a clipboard.
bool StartTheLeague(PlayerState& player, LeagueFormat format, int night,
                    const DayDials& dials = DayDials{});
std::string WhyNotStartTheLeague(const PlayerState& player,
                                 const DayDials& dials = DayDials{});

// **Run a week of it.** Three hours and ten energy on the night you chose,
// and you do not climb it -- you run it, the same way you run a circuit
// round. Six weeks make a run, and then somebody's name goes on the wall.
bool RunTheLeagueNight(PlayerState& player, DayState& day,
                       const DayDials& dials = DayDials{});
std::string WhyNotTheLeagueTonight(const PlayerState& player, DayState& day,
                                   const DayDials& dials = DayDials{});

// **GYM-9: put your gym forward to host the season's circuit rounds.**
// Answered on the spot, deterministically, and a rejection stands until the
// next season comes round -- the $2,200 deposit does not buy a second
// answer, and it is gone either way.
//
// Returns whether the season is yours. `WhyNotBid` on the gym says why you
// could not ask.
bool BidToHostTheSeason(PlayerState& player, const DayDials& dials = DayDials{});
std::string WhyNotBidToHost(const PlayerState& player,
                            const DayDials& dials = DayDials{});

// **Run the round.** The trade at circuit scale: on the day, you can run it
// or you can climb it, and **the host does not get a scorecard**. Nobody
// who has ever set a comp they were entered in would pretend otherwise --
// you know where every hold is.
//
// Six hours, eighteen energy, thirty-four entries at $26 a head, nine
// people who came to watch and came back on Tuesday, and the season's
// points go to somebody else.
bool RunTheRound(PlayerState& player, DayState& day,
                 const DayDials& dials = DayDials{});
std::string WhyNotRunTheRound(const PlayerState& player,
                              const DayDials& dials = DayDials{});

// **Found the youth team.** `GYM-2` asked the question -- Piper's mother
// wanting to know whether there is a waitlist -- and this is the answer, so
// it is gated on having actually lived that arc out. Four kids, a set of
// borrowed harnesses, and $1,200 of mats, kit and paperwork.
//
// Returns false with no gym, without Piper's story, if one already exists,
// or if you cannot pay. `WhyNoYouthTeam` says which.
bool FoundYouthTeam(PlayerState& player, const Rng& worldRng,
                    const DayDials& dials = DayDials{});
std::string WhyNoYouthTeam(const PlayerState& player,
                           const DayDials& dials = DayDials{});

// **An evening with the squad**, every third day, two hours and twelve
// energy. Every fifth one is a trip rather than a training night.
bool RunYouthSession(PlayerState& player, DayState& day, const Rng& worldRng,
                     const DayDials& dials = DayDials{});

// Hand the squad to somebody you pay, or take it back. The same trade the
// building makes one floor down, and worse for the kids either way you look
// at it: a paid coach brings them on more slowly and costs the gym $30 a
// day, and it is your evening back.
bool SetTheYouthCoach(PlayerState& player, bool hired, const Rng& worldRng);

// **Comp night.** Costs $150 and an evening, needs a room worth filling,
// and pays back in entry fees, walk-in signups and standing -- on a
// cooldown, because scarcity is the draw and an occasion you can hold every
// night is not an occasion.
//
// The night's story is one of the regulars whose arc you actually lived,
// which is why this is `GYM-2` and not a shop item: it is only worth
// hosting once there is somebody in the room to be the story.
bool HostCompNight(PlayerState& player, DayState& day, const Rng& worldRng,
                   const DayDials& dials = DayDials{});

// Take a gig off the board: its hours, its energy, its money. Returns false
// if it needs the van and the van is not going anywhere — which is how a
// breakdown costs you the fix and the work that would have paid for it.
//
// **The trade gets worked too**: the hours grow the craft, the craft moves
// the pay, and the hours teach your climbing a very little. `theHardWay`
// answers whatever came up on the shift -- see `MomentOnShift`. It defaults
// to the easy answer, which is never wrong and never gets you anywhere,
// so every existing caller keeps working and keeps meaning something.
bool WorkOddJob(PlayerState& player, DayState& day, const OddJob& job,
                const Rng& worldRng, bool theHardWay = false,
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
// probe-only: the probe's one-call shortcut for an indoor day, and the
// rule inside it -- IsGymMember -- *is* enforced in the played game, at the
// wall, which is what makes this a shortcut rather than a divergence. It
// was a real divergence until 2026-08-23: the check lived only here, the
// engine reached the gym by travel spot and wall, and the gym was free.
// unwired-ok: the probe's one-call shortcut for an indoor day. The engine
// reaches the gym the way a player does -- drive there, walk to a wall,
// press E -- and gates it on IsGymMember at the wall, so wrapping this
// would be a second path to the same place with its own copy of the rule.
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

// `CHAR-6`: has a style gone deep enough to have a name? Fills `out` and
// returns true when the scene is ready to call something after you --
// fifteen sends of one style for the first, forty of a *different* one for
// the second, and never a third.
//
// **Nothing is named without the player.** This only says one is ready; the
// name is theirs, which is the entire point of the feature and the only
// place in this game the player names their own climbing.
bool AMoveWantsAName(const PlayerState& player, RouteType& out,
                     const StyleDials& style = StyleDials{});

// Name it. False on an empty name or when nothing was ready. Bumps your
// standing with the scene once, because a move with a name is a thing
// people say about you.
bool NameTheMove(PlayerState& player, const std::string& name,
                 const StyleDials& style = StyleDials{});

// `CLB-32`: study the field before a comp that is on the calendar.
//
// Costs an evening's focus and nothing else -- no time, because film study
// is what you do instead of sleeping rather than instead of climbing. False
// when there is no comp coming, when you have already done it, or when you
// are too gassed to take anything in.
bool ScoutTheField(PlayerState& player, DayState& day,
                   const CompDials& comp = CompDials{});

// Why not, in the game's voice. Empty when you can.
std::string WhyNotScout(const PlayerState& player, const DayState& day,
                        const CompDials& comp = CompDials{});

// Carry the plan into the room. Called when a comp's board is set: the flag
// moves off the climber and onto the board, so it lasts exactly one comp.
void TakeTheScoutingIn(CompState& board, PlayerState& player);

// What the week cost, on the morning it landed. Empty on every other day
// -- read across Sleep, the way the tax bill and the World Cup's season
// news are.
std::string BillLine(const PlayerState& player);

// `DEPTH-19`: what the life costs at this grade. One at V0 and capped at
// three, so a V12 climber pays about twice what they did on arrival.
double LifestyleMultiplier(double grade, const DayDials& dials = DayDials{});

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

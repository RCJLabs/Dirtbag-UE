#pragma once

// Speed climbing -- the third Olympic discipline.
//
// `SPEED-1` through `SPEED-4` and `OLY-4`, ported from the 2D source. The
// Games have been in this port since `Sim/DirtbagWorldStage.h` and have run
// on two disciplines, which is not the format: an Olympic climbing programme
// is boulder, lead and **speed**, and speed is the one that is not climbing
// as this game otherwise means it. Fifteen metres, a fixed route every
// climber on earth has memorised, one lane each, two runs, fastest counts.
//
// ## Why it is a minigame and not a roll
//
// The source's own line is the brief: *"the whole event is the reaction
// start: wait for GO, then go."* Nothing about a speed run is a question of
// whether you can do the moves -- everyone in the final can do the moves.
// It is a start and a cadence, which makes it **the first thing in this port
// since HOLD TO CLIMB that the player actually plays** rather than watches
// the sim arbitrate.
//
// So the shape here is the shape `Sim/DirtbagSessionLoop.h` already
// established: a live core the presentation layer drives move by move, and
// a bot on top of it for every run nobody is playing -- the field's runs, an
// opponent's heat, a replayed career. `RunTime` is the whole physics and it
// takes what the player did, not what the player is.
//
// ## The four numbers a run is made of
//
// A false start is a DNF and there is no appeal. Past that: your reaction,
// your cadence error summed over sixteen rungs, and the rungs you fumbled.
// Your grade sets the pace you *could* hold; the beat decides whether you
// held it.
//
// ## What is deliberately not here
//
// The 2D game keys the comp room off `discRank[disc]`, so which tier you are
// allowed into depends on your ranking **in that discipline** -- see
// `RankingIn` in `Sim/DirtbagComp.h`. Its `natlPts` is then the *best* of the
// three, and this port keeps its own rolling sum instead. That is a logged
// deviation, not an oversight: 2D's ranking is a lifetime total, where a
// best-of-three is the only thing stopping three disciplines inflating one
// number. This port's ranking has been a one-year rolling window since the
// ladder shipped, and the three named rungs on it -- 700 for the national
// team, 1200 for the Games, 2200 for World-Class -- were measured against the
// sum. Taking the max would cut every existing career's ranking by about two
// thirds against thresholds nothing else moved.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

struct SpeedDials {
  // --- the wall ---------------------------------------------------------
  // Seconds a grade-0 climber takes on the 15m wall, and what each grade
  // shaves off it. The floor is the wall record: nobody beats it, however
  // strong they get, because the route is the same route for everybody and
  // it has a bottom.
  double baseSeconds = 16.0;
  double perGrade = 0.7;
  double floorSeconds = 4.6;

  // Points per second under `baseSeconds`. A comp score, not a ranking.
  double pointsPerSecond = 18.0;

  // What a false start or a blown run logs. Scores zero by construction,
  // and is a real number rather than a flag so a heat can compare times
  // without a special case for "did not finish".
  double dnfSeconds = 30.0;

  // Two runs in a round; your fastest counts.
  int runsPerRound = 2;

  // --- the start --------------------------------------------------------
  // The reaction that neither shaves nor adds time, the milliseconds of
  // reaction that are worth a second either way, and the cap. Capped
  // because without one a superhuman reaction beats the wall record, and
  // the wall record is the point of the floor.
  double reactionOkMs = 350.0;
  double reactionScaleMs = 300.0;
  double reactionCapSeconds = 1.5;

  // --- the cadence ------------------------------------------------------
  // Sixteen alternating hand-reaches, a window either side of each beat
  // that counts as clean, seconds added per second of cumulative error
  // past the window, and what a wrong-hand reach costs. A fumble does not
  // advance you, which is why it is priced above a merely late reach.
  int rungs = 16;
  double beatWindowMs = 110.0;
  double offBeatPenalty = 2.2;
  double fumbleSeconds = 0.22;

  // Milliseconds between beats, by grade: better climbers move faster up
  // the wall, so the beat they have to hold is tighter. Clamped both ends.
  double paceBaseMs = 780.0;
  double pacePerGrade = 30.0;
  double paceFloorMs = 320.0;
  double paceCeilingMs = 700.0;

  // --- the field --------------------------------------------------------
  // Spread of a field run and of a knockout opponent's heat around their
  // grade pace, and an opponent's per-heat false-start chance. The heat
  // spread is wider than the field's on purpose: one race against one
  // person is a coin-flip in a way a time trial is not.
  double fieldSpreadSeconds = 1.2;
  double heatSpreadSeconds = 1.4;
  double opponentFalseStart = 0.10;

  // How much nerve is worth on the start. A speed final is a clock and a
  // crowd and a person in the next lane, which is exactly what nerve is
  // for -- see Sim/DirtbagCharacter.h. Full nerve is worth this many
  // milliseconds off the reaction and this much off the false-start risk.
  double nerveReactionMs = 90.0;
  double botFalseStartBase = 0.09;
  double nerveFalseStart = 0.07;
  // Spread of a bot's reaction and of its cumulative cadence error.
  double botReactionSpreadMs = 220.0;
  double botOffBeatPerRung = 46.0;
  double botFumbleChance = 0.07;

  // --- the bracket ------------------------------------------------------
  // Top eight qualifiers seed a knockout: quarter, semi, then the final,
  // with the semi's losers racing for bronze.
  int bracketSize = 8;

  // --- practice ---------------------------------------------------------
  // A lap on the gym's speed wall. Energy is the whole limiter -- a run
  // takes twenty seconds, so charging time for it would be a lie -- and a
  // clean one trickles Power, with a share of it going to Technique
  // because the wall is a memorised sequence before it is a sprint.
  double practiceEnergy = 7.0;
  double practicePowerGain = 4.0;
  double practiceTechniqueShare = 0.4;
};

// The pace a grade can hold, and the time that pace is worth if nothing
// goes wrong. Clamped to the wall record.
double SpeedTimeForGrade(double grade, const SpeedDials& dials = SpeedDials{});
double SpeedPaceMs(double grade, const SpeedDials& dials = SpeedDials{});

// Comp points for a time. Floors at zero rather than going negative: a slow
// run scores nothing, it does not score against you.
double SpeedScore(double seconds, const SpeedDials& dials = SpeedDials{});

// What the player did on one run -- the beat's outputs, and the only inputs
// the clock reads. `offBeatMs` is the **cumulative** absolute error across
// the rungs, not the worst one: a run that is nine milliseconds late every
// time is a slow run, and one that is perfect fifteen times and catastrophic
// once is a different kind of slow.
struct SpeedRun {
  bool falseStart = false;
  double reactionMs = 350.0;
  double offBeatMs = 0.0;
  int fumbles = 0;
};

// The clock. This is the whole physics of a speed run.
double RunTime(double grade, const SpeedRun& run,
               const SpeedDials& dials = SpeedDials{});

// A run nobody played. The bot on the live core, exactly as `ResolveAttempt`
// is a bot on the session core -- so a career resolved headlessly and a
// career played by hand go through the same clock.
SpeedRun BotRun(const Rng& worldRng, const std::string& salt, double grade,
                double nerve, const SpeedDials& dials = SpeedDials{});

// A field runner's time, which is the 2D model exactly: their pace, jittered.
// Used for the qualifying field and for a knockout opponent, which differ
// only in spread.
double FieldTime(const Rng& worldRng, const std::string& salt, double grade,
                 bool heat, const SpeedDials& dials = SpeedDials{});

// --- a round ------------------------------------------------------------

// Two runs, fastest counts.
struct SpeedRound {
  std::vector<double> runs;
  double best = 0.0;    // 0 until a run is logged
  double score = 0.0;
};

// Log one run's time. Does nothing once the round is full.
void LogRun(SpeedRound& round, double seconds,
            const SpeedDials& dials = SpeedDials{});
bool RoundIsDone(const SpeedRound& round,
                 const SpeedDials& dials = SpeedDials{});

// --- the knockout -------------------------------------------------------

struct SpeedEntrant {
  std::string name;
  double grade = 0.0;
};

struct SpeedHeatLog {
  std::string label;
  std::string opponent;
  double yourSeconds = 0.0;
  double theirSeconds = 0.0;
  bool youWon = false;
  bool theyFalseStarted = false;
};

// One knockout. `round` is 0 quarter, 1 semi, 2 the last one -- which is the
// final unless `bronze`, because a semi you lost drops you into it rather
// than out of the building.
struct SpeedBracket {
  std::vector<SpeedEntrant> field;
  int round = 0;
  bool bronze = false;
  SpeedEntrant opponent;
  double opponentSeconds = 0.0;
  bool opponentFalseStarted = false;
  double yourSeconds = 0.0;
  bool resolved = false;   // this heat has a result the player has not read
  bool youWonIt = false;
  bool done = false;
  int placement = 0;       // 0 while live; 1 win, 2 final loss, 3 bronze, 4, 5 out at the quarter
  std::vector<SpeedHeatLog> log;
};

// What a heat is called.
const char* HeatName(int round, bool bronze);

// Seed the knockout from the qualifying field, best first, and roll the
// first opponent.
SpeedBracket SeedTheBracket(const Rng& rng, int day,
                            const std::vector<SpeedEntrant>& qualifiers,
                            double yourGrade,
                            const SpeedDials& dials = SpeedDials{});

// Race the heat. A false start loses it; two false starts and the slower
// clock still loses, which is why a DNF is a time. **Takes no rng** -- the
// opponent's time was rolled when the heat was drawn, so by the time you run
// it there is nothing left to decide.
void ResolveHeat(SpeedBracket& bracket, double yourSeconds,
                 const SpeedDials& dials = SpeedDials{});

// Move to the next heat once the player has read the last one. A win
// advances; a semifinal loss drops to the bronze match. Does nothing on a
// finished bracket or an unread heat.
void NextHeat(SpeedBracket& bracket, const Rng& rng, int day, double yourGrade,
              const SpeedDials& dials = SpeedDials{});

// What the heat reads like, in the game's voice.
std::string HeatLine(const SpeedBracket& bracket);

// --- the gym wall -------------------------------------------------------

struct SpeedPracticeResult {
  bool ran = false;          // false when there was not the energy for it
  bool clean = false;        // a false start burns the gas and teaches nothing
  bool personalBest = false;
  double seconds = 0.0;
  double powerGained = 0.0;
  double techniqueGained = 0.0;
};

// A lap on the speed wall, outside a comp. Trains, logs a personal best, and
// costs energy and nothing else.
SpeedPracticeResult PracticeRun(Skills& skills, double& energy,
                                double& personalBest, double seconds,
                                double trainingCeiling,
                                const SpeedDials& dials = SpeedDials{});

// What the PB reads like. Empty when there has never been a clean run --
// **not "0.00s"**, which is a wall record nobody has ever set.
std::string PersonalBestLine(double personalBest);

}  // namespace dirtbag

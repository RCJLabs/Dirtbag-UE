#pragma once

// The league — the low end of the same system, and the reason the gym is
// somewhere to be on a Wednesday.
//
// **A league is not a small comp.** A comp is a day with a result; a league
// is a habit with a number. It costs five dollars, it runs every week
// forever, it pays almost nothing, and **it is worth no ranking points at
// all** — which is the whole design. What you chase is your own best score,
// and that is a different kind of goal from a placing: it only ever goes
// up, nobody else can take it off you, and beating it by one point on a
// Wednesday in a bad year is still beating it.
//
// The other half is the block. Eight weeks make a block, the block has a
// table, and winning one is a small thing that is genuinely yours. It gives
// the habit an ending, which is what stops a weekly event from being
// wallpaper.
//
// ## Why enter, given it pays nothing
//
// Because the board is five problems somebody else set at your grade, and
// you would not have chosen them. That is training, and the attempts train
// you exactly as a comp's do. The number is the excuse.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagComp.h"
#include "DirtbagRng.h"

namespace dirtbag {

struct LeagueDials {
  // **Every week, on the same night.** Firm, like the circuit's dates and
  // for the same reason: a thing you can plan around is a decision, and a
  // thing that turns up at random is weather.
  int everyDays = 7;

  // Five dollars. It has to be small enough that skipping it is never
  // about money, or the league becomes a budget decision and it is not
  // one.
  // **Named apart from `CompDials::entryFee` and the four below it on
  // purpose.** These are deliberately different numbers from a comp's --
  // that is the entire point of a league -- and the dial checker reads two
  // dials of one name as one number that has drifted. It caught
  // `RivalDials::startAge` and the World Cup's calendar the same way, and
  // in both cases the deliberate gap was indistinguishable from a bug
  // until the name said so.
  double nightFee = 5.0;

  // Five problems and **ten goes**, which is more than a comp gives you.
  // A league night is a session with a scorecard, not a test: the point is
  // that you climbed, and a stingy attempt count would make it a worse
  // comp rather than a different thing.
  int nightProblems = 5;
  int nightAttempts = 10;
  double nightHours = 3.0;
  double nightEnergy = 20.0;

  // **A spread, not five at your level** -- from well below you to above,
  // because a gym sets for the whole room and the whole room turns up on a
  // Wednesday.
  double spreadLow = -3.0;
  double spreadHigh = 2.0;

  // **Points by the grade of the problem, not by where it sits on the
  // board.** This is the dial that makes a personal best worth chasing.
  //
  // Priced the comp way -- worth-relative-to-the-board -- the maximum
  // score is the same every week however good you get, so the number
  // saturates the first month and then never moves again. Measured: **two
  // personal bests in five hundred and nineteen league nights.** Priced by
  // grade, the board rises with you and so does the number, which is the
  // whole reason to keep turning up.
  double pointsPerGrade = 5.0;

  // Eight weeks to a block. Long enough that one bad Wednesday is not the
  // whole thing and short enough to finish.
  int weeksPerBlock = 8;

  // What it pays, which is nearly nothing on purpose. **A league is not
  // income.** The night's winner gets beer money and a block is worth a
  // week of shifts, and if either number were larger the gym would become
  // a job with a scorecard.
  double nightCash = 40.0;
  double blockCash = 80.0;

  // In standing units, where Sim/DirtbagFactions.h says 0.1 is a small
  // deliberate act and 0.3 is a big one. **Both of these are deliberately
  // under a small act**: the scene notices, and it does not care much.
  double bestRep = 0.05;
  double blockRep = 0.08;

  // The regulars. Not the comp field -- these are the people who are
  // always there on a Wednesday, and they are worse than the comp field
  // because the good ones are at a comp on a Saturday.
  //
  // Half a grade, not a grade and a half: at -1.5 the probe won
  // **forty-seven blocks out of sixty-five** while winning one domestic
  // comp in two hundred and fifty-three, which made the league the one
  // room in the game nobody could lose in.
  double regularShift = -0.5;
};

// A running league. Career state; **the night itself is not saved**, same
// rule a comp lives under: you are at the gym for three hours and the save
// is written when you sleep.
struct League {
  // The next night. Zero means the gym has not told you about it yet.
  int nextNight = 0;
  int block = 1;
  int weeksDone = 0;          // in this block
  double yourPoints = 0.0;    // in this block
  std::vector<double> fieldPoints;   // parallel to TheRegulars()

  // **What you are actually chasing.** Best single-night score, ever, and
  // the day you did it. It only goes up, and nobody can take it off you.
  double best = 0.0;
  int bestOnDay = 0;

  int nights = 0;      // career
  int blockWins = 0;

  // The last night you actually climbed. **Without it the regulars are
  // paid twice on the weeks you turn up** -- once off the scorecard by
  // `SettleLeague` and again by the night tick, which has no other way to
  // know whether anybody watched.
  int lastClimbedNight = 0;
};

// The regulars, six of them. Named, because after eight weeks you know who
// they are, and that is the point of a league.
//
// **`LeagueRegular` rather than `Regular`**, because `DirtbagPartner.cpp`
// has a file-local `Regular` already and it means somebody at the Lot. Two
// structs of that name in one translation unit is an ambiguity error today
// and a worse kind of confusion the day it is not -- the same collision
// `WorldCupVenue` had with the town's `Venue`, in the same week.
struct LeagueRegular {
  const char* name;
  double gradeOffset;
  RouteType signature;
  RouteType weakness;
  const char* who;    // one line, because a regular is a person
};
const std::vector<LeagueRegular>& TheRegulars();

// Is it on tonight?
bool LeagueTonight(const League& l, int day);
// Days until the next one, or -1 before the gym has scheduled any.
int DaysUntilLeague(const League& l, int day);

// One night of the league's own clock: schedule the first one, roll it
// forward once it has been and gone, and close a block when its eighth
// week is done. Returns the block's news, empty on a quiet night.
struct LeagueNight {
  bool blockClosed = false;
  int place = 0;
  bool won = false;
  double cash = 0.0;
  double rep = 0.0;
  std::string news;
};
LeagueNight LeagueDay(League& l, const Rng& worldRng, int day,
                      const LeagueDials& dials = LeagueDials{});

// Set tonight's board. Five at your grade, deterministic on the day.
CompState SetTheLeagueBoard(const Rng& worldRng, double yourGrade, int day,
                            const LeagueDials& dials = LeagueDials{});

// The comp dials a league night is resolved under. Same engine, softer
// room: more goes and a weaker field.
CompDials LeagueCompDials(const LeagueDials& dials = LeagueDials{});

// Turn in the card. Banks the block points, moves your best if you moved
// it, and pays the night's winner beer money.
struct LeagueResult {
  int place = 0;
  int fieldSize = 0;
  double score = 0.0;
  bool personalBest = false;
  double improvedBy = 0.0;
  double cash = 0.0;
  double rep = 0.0;
  std::vector<CompEntrant> board;
  std::string news;
};
LeagueResult SettleLeague(League& l, const CompState& comp, double yourGrade,
                          int day, const Rng& compRng,
                          const LeagueDials& dials = LeagueDials{});

// The block table, best first.
std::vector<CircuitStanding> LeagueTable(const League& l);

// What the gym has on the whiteboard. Empty when there is nothing to say.
std::string LeagueLine(const League& l, int day,
                       const LeagueDials& dials = LeagueDials{});

}  // namespace dirtbag

#pragma once

// The comp, and why it is worth porting at all.
//
// **The format is the point.** A gym comp is *five problems, seven attempts
// across the whole session, and an eighty-second clock* -- you spend
// attempts, not time, and you bank what you can before it runs out. That is
// a resource-allocation minigame played with verbs the session resolver
// already has, which is CLAUDE.md's core design call word for word: **2D
// minigames, 3D staging.** A comp that resolved to a number would be a
// table lookup with a rosette on it.
//
// Reinstated by `concepts/DECISION-comps-are-back.md`. §4 cut comps past
// 1.0; Evan: *"wait comps were cut??? ok bring them back and the olympics.
// that's a huge part of the game."* The reasoning is in the decision doc --
// the short version is that a climbing career here has two arcs and the port
// had one, and this is the half with an ending.
//
// ## What this file is, and what it is not yet
//
// This is **the comp itself**: the problems, your session, the field you are
// scored against, the tiers, the placing and the payout. The things that sit
// *on top* of a comp -- a circuit season, national ranking, the team, the
// World Cup, the Games -- read a result and are the next pass. They are
// listed in ROADMAP Phase 9 and they all key off `CompResult`.
//
// ## Two rules carried over already-paid-for
//
// **The field is a redistribution, not a difficulty setting.** Seven named
// climbers at stable offsets from you, each with **a discipline they are
// known for and one they are soft on** -- a grade harder and a grade softer.
// Every competitor gets exactly one of each, and a comp sets a spread across
// the types, so the field's average strength is unchanged. What changes is
// that results start to mean something: Kai takes the dyno problem off you
// every time and you take the crimpy one back off him.
//
// **The offsets sit below you, not around you.** The 2D game shipped them
// symmetric -- three above, three below, plus the rival -- and measured
// **5th on average, 2.2% podiums and 0% wins, forever**, which gated three
// downstream systems behind an event that happened one comp in fifty. They
// are shifted down two here for the same reason. **Local is yours to lose,
// Regional is a fight, National is the mountain.**
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"
#include "DirtbagSession.h"

namespace dirtbag {

// Where a comp sits, and what turns up to it.
enum class CompTier { Local, Regional, National };
constexpr int kCompTierCount = 3;
const char* TierName(CompTier t);

// One problem on the board.
struct CompProblem {
  Route route;
  RouteType type = RouteType::Power;
  // What it is worth. A flash is worth more than a top, and a top is worth
  // more than getting to the zone -- the whole reason to spend your
  // attempts carefully rather than evenly.
  double topPoints = 10.0;
  double flashPoints = 13.0;
  double zonePoints = 4.0;
};

// How you did on one problem.
struct ProblemProgress {
  int tries = 0;
  bool topped = false;
  bool flashed = false;   // topped first go
  int zone = 0;           // 0 none, 1 low zone, 2 high zone
};

// A comp in progress. The player spends `attemptsLeft` across the board.
struct CompState {
  CompTier tier = CompTier::Local;
  std::vector<CompProblem> problems;
  std::vector<ProblemProgress> progress;
  int attemptsLeft = 0;
  bool finished = false;
};

// Somebody on the scoreboard.
struct CompEntrant {
  std::string name;
  double score = 0.0;
  bool isYou = false;
  bool isRival = false;
};

struct CompResult {
  int place = 0;            // 1 is first
  int fieldSize = 0;
  double yourScore = 0.0;
  bool beatTheRival = false;
  double cash = 0.0;
  double rep = 0.0;
  std::vector<CompEntrant> board;   // sorted, best first
};

struct CompDials {
  int problems = 5;
  // **Seven, across the whole session.** Not seven each -- that is the
  // mechanic. Five problems and seven goes means at most two can be worked
  // and the rest are one-shot, so the first decision of a comp is which two
  // you believe in.
  int attempts = 7;

  // A comp keeps this much of your normal margin; nerves eat the rest.
  // 0.8 from the 2D game, and it is the single dial that makes a comp feel
  // different from a session on the same holds.
  double pressure = 0.80;

  // Form on the day. **Every competitor climbs a little above or below
  // themselves**, which is what makes a comp a contest rather than a table
  // lookup. The 2D game shipped without it on the rival specifically, so he
  // posted his theoretical maximum in every qualifier he ever entered while
  // everyone else had off days.
  double formLow = 0.79;
  double formRange = 0.42;   // 0.79x .. 1.21x

  // A grade harder on your signature type, a grade softer on your weakness.
  double signatureShift = 1.0;

  // Where the problems sit relative to you, per tier. Local is at your
  // level, National is above it.
  double localGradeOffset = 0.0;
  double regionalGradeOffset = 1.0;
  double nationalGradeOffset = 2.0;

  // **And who turns up, per tier -- which is the dial that makes the three
  // mean three different things.**
  //
  // Measured before it existed: flashing an *entire* local board won 16
  // times in 60, because the field's offsets are relative to you and Kai
  // sits a grade above. A competitor a grade up flashes everything the
  // board has, so their ceiling equals a perfect round's and **form decides
  // the winner** -- you cannot beat them by climbing better, only by them
  // having a bad day. That is not "Local is yours to lose", it is a coin
  // toss with extra steps.
  //
  // Shifting the field by tier is what the 2D game does (its `COMP_TIERS`
  // carries a field column, 0/1/2) and it is the honest fix: the same seven
  // people, a weaker crowd at the local one.
  // **One, and the dial is coarser than it looks.** Swept: -0.5, -0.75,
  // -1.0 and -1.25 all give the same answer, because grades are integers
  // and a competitor's effective level is compared against them in
  // half-grade bands -- the shift only does anything when it crosses a
  // boundary. -1.0 is the middle of that plateau and reads as what it is:
  // the local crowd is a grade weaker than the one you meet at Regional.
  //
  // Measured at -1.0: flashing an entire local board wins **48 of 60** and
  // podiums 60 of 60. That is "yours to lose" -- a perfect round takes it
  // four times in five and Kai still steals one, which is the difference
  // between a comp and a formality.
  double localFieldShift = -1.0;
  double regionalFieldShift = 0.0;
  double nationalFieldShift = 1.0;

  // What a tier costs to enter and what the day takes.
  double entryFee = 20.0;
  double hours = 6.0;
  double energy = 30.0;

  // Ranking points needed to be allowed into each tier.
  double regionalAt = 350.0;
  double nationalAt = 1200.0;

  // The zones, as a fraction of the problem's moves. Reaching two thirds of
  // the way up scores; reaching a third scores less.
  double lowZoneAt = 0.34;
  double highZoneAt = 0.67;

  // Placement money and reputation. Deliberately small: **comp prize money
  // is not how a climber eats**, and the 2D game's own note on this says
  // the circuit had quietly become economically pointless when it tried to
  // be. What a comp pays in is standing.
  double winCash = 120.0, winRep = 10.0;
  double podiumCash = 60.0, podiumRep = 5.0;
  double topHalfCash = 25.0, topHalfRep = 2.0;
  double showedUpRep = 1.0;
  // Beating the rival is worth its own bump, on top of wherever you placed.
  double beatRivalRep = 4.0;

  // How often one lands, and how much warning you get.
  //
  // **A fortnight, announced three days out.** The warning is the mechanic:
  // a comp you find out about on the day is a dice roll, and one you can
  // see coming is a week of deciding whether to rest for it. Three days is
  // enough to skip a session and not enough to train for it.
  int everyDays = 14;
  int announceDaysAhead = 3;
};

// The day of the next comp on or after `today`. Comps land on a fixed
// cadence rather than being rolled, because a schedule you can plan around
// is the entire difference between a comp and a random event -- and the 2D
// game learned that too, where the circuit's dates are firm and no-showing
// one costs you.
int NextCompDay(int today, const CompDials& dials = CompDials{});

// Is there one today?
bool CompIsToday(int today, const CompDials& dials = CompDials{});

// How many days until the next one, or -1 if it is further off than the
// announcement window. This is what the gym has on a poster.
int DaysUntilComp(int today, const CompDials& dials = CompDials{});

// Which tier you are allowed into, from your national ranking points.
CompTier TierFor(double rankingPoints, const CompDials& dials = CompDials{});

// Set the board. Five problems spread across the comp types at the tier's
// grade, deterministic from (seed, day) so the same comp is the same comp
// on a reload -- the same no-reroll rule every gamble in this game lives
// under.
CompState SetTheBoard(const Rng& worldRng, CompTier tier, double yourGrade,
                      int day, const CompDials& dials = CompDials{});

// Spend one attempt on one problem. Returns the resolved attempt so the
// presentation layer has its timeline, exactly as a session burn does.
// Does nothing and returns an empty result when the attempt is not legal --
// no attempts left, a bad index, or a problem you have already topped.
AttemptResult AttemptProblem(CompState& comp, int problemIndex,
                             const Climber& climber, const Rng& compRng,
                             const CompDials& dials = CompDials{});

// What you have banked so far.
double YourScore(const CompState& comp, const CompDials& dials = CompDials{});

// The named field. Seven of them, each with a signature type and a weakness.
struct Competitor {
  const char* name;
  double gradeOffset;
  RouteType signature;
  RouteType weakness;
};
const std::vector<Competitor>& TheField();

// What a competitor of this grade scores on this board. Deterministic given
// the form roll, so the scoreboard can be rebuilt rather than stored.
double CompetitorScore(const std::vector<CompProblem>& problems, double grade,
                       RouteType signature, RouteType weakness, double form,
                       const CompDials& dials = CompDials{});

// Rank everybody and pay out. `rivalName` empty means no rival entered.
CompResult Settle(const CompState& comp, double yourGrade,
                  const std::string& rivalName, double rivalGrade,
                  const Rng& compRng, const CompDials& dials = CompDials{});

// How it reads, in the game's voice.
std::string PlacingText(const CompResult& r);

}  // namespace dirtbag

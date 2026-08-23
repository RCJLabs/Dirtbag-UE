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

#include "DirtbagBodyContext.h"
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

// Which round of a comp is on the wall.
//
// **A Tuesday at the gym is one board and done; a Regional is a day.**
// That is the whole difference the format makes: at a tiered comp you have
// to do it three times, the score does not carry, and a good qualification
// buys you a place in the semi rather than a placing.
enum class CompRound { Qualification = 0, Semi, Final };
constexpr int kCompRoundCount = 3;
const char* RoundName(CompRound r);

// A comp in progress. The player spends `attemptsLeft` across the board.
struct CompState {
  CompTier tier = CompTier::Local;
  std::vector<CompProblem> problems;
  std::vector<ProblemProgress> progress;
  int attemptsLeft = 0;
  bool finished = false;

  // Which round this board is. Qualification for a Local comp, and it
  // never moves off it.
  CompRound round = CompRound::Qualification;
  // Who is still in it, by name, **including "You"**. Empty means the
  // whole field, which is what qualification is. `Settle` reads this and
  // ranks nobody who has been cut -- without it a semi-final would be
  // scored against six people who went home.
  std::vector<std::string> stillIn;
};

// Is this name still in the comp? A comp with nobody cut yet says yes to
// everybody, which is what makes qualification the same call as a final.
bool StillIn(const CompState& comp, const std::string& name);

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

  // How much warning you get. **Three days**, and the warning is the
  // mechanic: a comp you find out about on the day is a dice roll, and one
  // you can see coming is a week of deciding whether to rest for it. Enough
  // to skip a session and not enough to train for it.
  int announceDaysAhead = 3;

  // --- quals, semi, final ----------------------------------------------
  //
  // **The lowest tier that runs a real three-round comp.** Below it a comp
  // is one board: a gym comp on a Tuesday is not a day off work, and
  // making somebody climb three rounds for a $20 local would be the format
  // doing the opposite of what it is for.
  CompTier roundsFrom = CompTier::Regional;

  // How many come out of each round. The field is eight or nine, so six
  // and four -- an isolation zone and then a real final, at the scale a
  // national scene actually runs.
  int semiCut = 6;
  int finalCut = 4;

  // **A final is shorter and harder.** Four problems and fewer goes, so a
  // single mistake is the whole result -- which is what a final is.
  int finalProblems = 4;
  int finalAttempts = 5;

  // Each round sits above the last. Half a grade, because the field is
  // getting thinner at the same time and both together would make a final
  // unclimbable rather than hard.
  double roundGradeStep = 0.5;

  // What another round takes out of you. Charged per round rather than
  // once for the day: three rounds is three times the pump, and that is
  // the cost of the format.
  double roundEnergy = 20.0;

  // --- the circuit season ---------------------------------------------
  // Five comps, and **the last one is the finals**. Not a sixth event: the
  // same format worth half as much again, so a season has a shape rather
  // than being five identical Tuesdays.
  int compsPerSeason = 5;
  double finalsMultiplier = 1.5;
  // Firm dates, six to eight days apart. Firm is the point -- a schedule
  // you can plan around is the whole difference between a comp and a random
  // event, and it is what makes not turning up a *decision*.
  int gapMin = 6;
  int gapVariance = 3;
  // And the wait between seasons, which is what a close season is for.
  int seasonBreakDays = 40;

  // What a season pays at the top. **Larger than a comp by a lot**, because
  // this is the thing a year of turning up is for -- and still small next
  // to a sponsor, because comp prize money is not how a climber eats.
  double championCash = 300.0, runnerUpCash = 100.0, bronzeCash = 50.0;
  // Ranking points on top of the placement points, for the season table.
  double championRanking = 150.0, runnerUpRanking = 80.0,
         bronzeRanking = 40.0;

  // **What not turning up costs.** The rival banks this, and you lose
  // standing for it: a firm schedule you can ignore for free is not a
  // commitment, it is a suggestion.
  double forfeitRivalPoints = 60.0;
  double forfeitRankingLoss = 2.0;

  // **The shape of the placement curve.** Each place down is worth this
  // much of the one above: 100, 70, 49, 34, 24, 17, 12 ... floored at 5.
  // Set by measurement -- at 0.78 a permanent mid-fielder still banked
  // nine hundred points a year and walked onto the national team without
  // ever beating anybody.
  double placeFalloff = 0.70;

  // **What room you did it in.** A placing was worth the same whatever
  // tier the comp was, which is the third and last piece of the same
  // inflation: a climber who could only podium at the gym banked the same
  // points as one podiuming at Nationals, so the tier system promoted them
  // into a room they could not place in and the ranking never noticed.
  //
  // Every real ranking table weights by event category and so does this
  // one. It is also what makes the tier feedback loop *settle*: promotion
  // means worse placings, and only the multiplier makes climbing the rung
  // worth it, so a career finds the level it belongs at instead of
  // oscillating.
  // Set so the rungs land where the 2D game's thresholds already are.
  // Over a year of turning up (about twenty-five comps): a Local podium
  // regular sits near Regional Climber, a Regional podium regular near the
  // national team's 700, a National podium regular near the Olympic gate's
  // 1200, and somebody winning Nationals near World-Class.
  double localRankingWeight = 0.30;
  double regionalRankingWeight = 0.65;
  double nationalRankingWeight = 1.00;

  // --- the ranking ladder ----------------------------------------------
  // Six named tiers. The numbers are the 2D game's and they are load-bearing
  // further up: 700 is where a national team calls you and 1200 is where the
  // Games become reachable, so moving them moves two systems that are not
  // built yet.
  // **How long a result counts for.** A year, like every ranking table in
  // the sport: what you did last season is what you are ranked on, and a
  // year away drops you off it. This is what makes "a career can fail to
  // reach the Games" true -- without it every career reaches them, because
  // points only ever went up.
  int rankingWindowDays = 365;

  double regionalClimberAt = 120.0;
  double nationalProspectAt = 350.0;
  double nationalTeamAt = 700.0;
  double olympicHopefulAt = 1200.0;
  double worldClassAt = 2200.0;
};

// Where you stand nationally. **Not the comp tier** -- that is which room
// you are allowed into, and this is what the room says about you. They share
// two numbers on purpose (350 lets you into Regional comps *and* makes you a
// National Prospect) and diverge above that.
enum class RankTier {
  Unranked,
  RegionalClimber,
  NationalProspect,
  NationalTeam,
  OlympicHopeful,
  WorldClass,
};
constexpr int kRankTierCount = 6;
const char* RankName(RankTier t);
RankTier RankFor(double rankingPoints, const CompDials& dials = CompDials{});
// How many points to the next one, or -1 at the top. What a progress bar
// would say, said as a number the caller can put in a sentence.
double ToNextRank(double rankingPoints, const CompDials& dials = CompDials{});

// Placement points. **1st takes 100, a podium takes about half of it, and
// mid-field takes a quarter.**
//
// The curve was linear in how many people you beat, and that was the single
// number behind the ladder not being a ladder: coming *fifth of nine* paid
// fifty, half a win, so a climber who never won anything and simply turned
// up twenty-five times a year banked twelve hundred points and cleared the
// Olympic gate. Measured at ten years: a ranking peak of **14,633 against a
// top tier of 2,200.**
//
// Top-weighted now, the shape every real ranking table has and the shape
// this project already wrote once for the World Cup. `fieldSize` no longer
// scales it -- the domestic board is always eight or nine people, so a
// field-size term was a constant wearing a dial's clothes -- and is kept
// only to reject a placing that is off the end of the field.
double CircuitPoints(int place, int fieldSize,
                     const CompDials& dials = CompDials{});

// --- the ranking is a record, not a total -----------------------------

// One result, and the day it happened. **The ranking is made of these and
// is not accumulated**, which is the second half of the same fix: a
// lifetime total means a tier cleared once is cleared forever, and the
// named rungs stop meaning anything about the climber you are *now*. Real
// ranking tables are a rolling window and so is this one.
struct RankingResult {
  int day = 0;
  double points = 0.0;   // negative for a no-show
};

// Put a result on the record, and drop anything that has aged out. Pruning
// here rather than at read time keeps the save bounded: a thirty-year
// career would otherwise carry seven hundred results it can never count.
void Record(std::vector<RankingResult>& record, int day, double points,
            const CompDials& dials = CompDials{});

// What the record adds up to today. Floors at zero -- a run of no-shows
// makes you unranked, not negative.
double RankingFrom(const std::vector<RankingResult>& record, int today,
                   const CompDials& dials = CompDials{});

// What a comp adds to your national ranking: the placement points weighted
// by which room you were in, half as much again at a finals, plus a lump
// for a season podium.
//
// **The tier is not optional and has no default.** Leaving it off is how a
// gym podium came to be worth a National one.
double RankingPointsFor(int place, int fieldSize, CompTier tier, bool finals,
                        bool champion, bool runnerUp, bool bronze,
                        const CompDials& dials = CompDials{});

// The weight that tier carries on the ranking.
double TierRankingWeight(CompTier tier, const CompDials& dials = CompDials{});

// Where everybody is in the season.
struct CircuitStanding {
  std::string name;
  double points = 0.0;
  bool isYou = false;
  bool isRival = false;
};

// A season of the circuit.
struct Circuit {
  int season = 0;          // 1-based; 0 means none has started
  int compsDone = 0;
  std::vector<int> schedule;   // firm dates, in order; the last is the finals
  double yourPoints = 0.0;
  double rivalPoints = 0.0;
  std::vector<double> fieldPoints;   // parallel to TheField()
  int titles = 0;          // seasons won, across a career
};

// Open a season. Five firm dates starting a few days out.
Circuit StartSeason(const Rng& worldRng, int day, int season,
                    const CompDials& dials = CompDials{});

// Is one of this season's comps today, and is it the finals?
bool CompIsToday(const Circuit& c, int day);
bool FinalsToday(const Circuit& c, int day);

// Days until the next one, or -1 outside the announcement window.
int DaysUntilComp(const Circuit& c, int day,
                  const CompDials& dials = CompDials{});

// Has the season's last comp been and gone?
bool SeasonOver(const Circuit& c, const CompDials& dials = CompDials{});

// How many of the season's dates are on or before `day`. The number of
// comps that *should* have been resolved by now -- compare it against
// `compsDone` to find a no-show, which is the only way to tell one from a
// comp you entered: both leave the date in the past.
int CompsDueBy(const Circuit& c, int day);

// Bank a result. Everybody scores: you from your placing, the field and the
// rival from theirs, so the table is a season rather than your season.
void BankResult(Circuit& c, const CompResult& result, bool finals,
                const CompDials& dials = CompDials{});

// You did not turn up. **The rival banks for it and you lose standing** --
// a firm schedule you can ignore for free is a suggestion.
void Forfeit(Circuit& c, std::vector<RankingResult>& record, int day,
             const CompDials& dials = CompDials{});

// The season's table, best first.
std::vector<CircuitStanding> SeasonTable(const Circuit& c);

struct SeasonEnd {
  int place = 0;
  double cash = 0.0;
  double rankingPoints = 0.0;
  bool title = false;
  std::vector<CircuitStanding> table;
};

// Close it out and pay the podium.
SeasonEnd CloseSeason(const Circuit& c, const CompDials& dials = CompDials{});

// What the standings read like, in the game's voice.
std::string CircuitLine(const Circuit& c, const CompDials& dials = CompDials{});

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
// `body` is everything the climber carries in with them -- the flaw, the
// temperament, the joints, the flu, the tooth, the rubber. **Defaulted to a
// clean body, which is neutral**, so every existing caller and every golden
// vector resolves exactly as it did; and a clean body is exactly what the
// comp resolver used to assume of everybody, which is the bug this argument
// exists to close. See Sim/DirtbagBodyContext.h.
AttemptResult AttemptProblem(CompState& comp, int problemIndex,
                             const Climber& climber, const Rng& compRng,
                             const CompDials& dials = CompDials{},
                             const BodyContext& body = BodyContext{});

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

// Does this tier run rounds at all?
bool RunsRounds(CompTier tier, const CompDials& dials = CompDials{});

// How many survive this round. Zero for a round that ends the comp.
int SurvivorsOf(CompRound round, const CompDials& dials = CompDials{});

// What happened at the end of a round. **Getting cut is a result, not an
// error** -- being out in qualification is a different day from finishing
// last in a final, and the game has to be able to say which.
struct RoundOutcome {
  bool through = false;
  int place = 0;          // where you came in the round just climbed
  int survivors = 0;
  CompRound next = CompRound::Qualification;
  std::string news;
};

// Close a round: cut the field, and either set the next board or leave the
// comp finished. Mutates `comp` -- a new board, a fresh scorecard, a fresh
// set of attempts, and the shorter list of who is left.
RoundOutcome NextRound(CompState& comp, const CompResult& result,
                       const Rng& worldRng, double yourGrade, int day,
                       const CompDials& dials = CompDials{});

// Rank everybody and pay out. `rivalName` empty means no rival entered.
CompResult Settle(const CompState& comp, double yourGrade,
                  const std::string& rivalName, double rivalGrade,
                  const Rng& compRng, const CompDials& dials = CompDials{});

// How it reads, in the game's voice.
std::string PlacingText(const CompResult& r);

}  // namespace dirtbag

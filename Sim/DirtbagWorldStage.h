#pragma once

// The World Cup and the Games — the top of the ladder.
//
// Both are the comp engine with a harder field and a plane ticket, and both
// gate on the ranking number Phase 9's second pass made real. What makes
// them different from a domestic comp is not the format:
//
// **The World Cup is a budget problem.** Ten real venues with real travel
// costs, six rounds a season, and **the field flies whether you do or
// not** — so a round you skip is not a round that did not happen, it is
// everybody else banking while you stayed home. That asymmetry is the
// entire mechanic: the season is decided by which rounds you could afford.
//
// **The Games are a ceiling.** They come round on a cycle rather than a
// schedule, you have to be an Olympic Hopeful to be there at all, the
// problems sit above your grade on purpose, and the field is the best in
// the world — so a medal is hard-won and an appearance is worth something
// by itself.
//
// ## What is deliberately shared with `DirtbagComp`
//
// The board, the attempts, the scoring, the pressure. A World Cup round is
// five problems and seven goes, exactly like a Tuesday at the gym, because
// **the format is not what makes it hard** — the field is, and the grade
// bump is, and getting there is. Building a second resolver for the same
// verbs is how two comps start disagreeing about what a flash is worth.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagComp.h"
#include "DirtbagRng.h"
#include "DirtbagTeam.h"

namespace dirtbag {

// ---------------------------------------------------------------------
// The World Cup
// ---------------------------------------------------------------------

// Named `WorldCupVenue` rather than `Venue`, because `DirtbagTown.h`
// already has one and it means something else entirely: a town venue is a
// place at home that opens at nine, and this is a city you fly to. Two
// structs called `Venue` in one namespace is a compile error today and a
// worse kind of confusion the day it is not.
struct WorldCupVenue {
  const char* city;
  const char* country;
  Discipline discipline;
  // What getting there costs. **The dial that makes a season a budget
  // problem**: Salt Lake is a domestic ticket and Seoul is most of a
  // month's money.
  double travel;
  const char* blurb;
};
const std::vector<WorldCupVenue>& TheVenues();

struct WorldCupRound {
  int day = 0;
  int venue = 0;   // index into TheVenues()
  bool resolved = false;
  bool flown = false;   // you actually went
};

struct WorldCupSeason {
  int season = 0;   // 0 = you have never had one
  std::vector<WorldCupRound> schedule;
  double yourPoints = 0.0;
  // The international field's points this season. **They fly whether you
  // do or not**, which is why this is banked per round rather than derived
  // at the end.
  std::vector<double> fieldPoints;

  // Career.
  int starts = 0;    // rounds you got on a plane for
  int missed = 0;    // rounds that went ahead without you
  int finals = 0;
  int podiums = 0;
  int wins = 0;
  int titles = 0;
  int bestRank = 0;  // best end-of-season world ranking, 0 = none
  int lastRank = 0;
  // Whether the table has been read out. A season sits closed through the
  // whole off-season, and without this the night tick would re-close it
  // every morning of the break -- which is a title announced forty-five
  // times.
  bool closed = false;
};

// Which ladder a live board belongs to. The engine holds one comp at a time
// and the settle has to know where the result goes: a World Cup round
// banked into the domestic circuit is a title nobody won.
enum class Stage { Domestic = 0, WorldCup, Games };

struct WorldStageDials {
  // --- the World Cup ---------------------------------------------------
  int rounds = 6;
  // **Named apart from `CompDials::gapMin` on purpose.** These are
  // deliberately different numbers -- a World Cup year is tighter than a
  // domestic one, four days between rounds against six -- and the dial
  // checker reads two dials of the same name as one number that has
  // drifted. It caught `RivalDials::startAge` shadowing `AgeDials::startAge`
  // the same way, and the deliberate gap was indistinguishable from a bug
  // until the name said so.
  //
  // **A World Cup season is a year, not a quarter.** At 4 and 4 with a
  // 45-day break the probe ran forty-seven seasons and two hundred and
  // eighty-one rounds in a ten-year career -- a World Cup every eleven
  // days, which is not a circuit, it is a treadmill. Six rounds at
  // fourteen to twenty-three days apart is about a hundred days of season;
  // the break below makes up the rest of the year.
  int roundGapMin = 14;
  int roundGapVariance = 10;
  int publishedDaysAhead = 5;
  // The off-season. Long, because that is what an international year is:
  // about a hundred days of competing and the rest of the year training
  // for it. Named apart from `CompDials::seasonBreakDays` for the reason
  // above -- they are deliberately different numbers.
  int worldSeasonBreakDays = 240;
  // On top of the comp's own six hours: the airport before and the airport
  // after. **There is no multi-day trip and that is deliberate** -- the day
  // loop has no "away" state, and inventing one so a plane ride can span
  // two mornings is a bigger change than the round is worth. What being
  // away actually costs is in the two numbers below.
  double travelHours = 4.0;
  // Airports, time zones, a bad night on a hotel mattress. Charged on top
  // of the comp's own energy, which is what makes a round abroad more
  // expensive than the same comp at home.
  double tripEnergy = 20.0;
  double tripHunger = 18.0;
  // **The world stage is absolute, and a domestic comp is not.**
  //
  // This is the load-bearing difference between this file and
  // `DirtbagComp`, and getting it wrong made the entire system
  // unwinnable at every skill level in the game: a gym comp is set at
  // *your* grade plus a tier offset, because a gym comp is your peers, so
  // the board follows you up as you improve. The World Cup does not. It is
  // set where it is set, the field climbs what it climbs, and getting
  // better is what closes the gap.
  //
  // **Set by measurement, not by feel.** A ten-year career at the crag
  // lands on an allround grade of about 6.9 (greedy 6.92, kitted 6.98,
  // sponsored 6.84 over 3650 days), so the world stage sits a grade and a
  // half above where a good outdoor decade finishes: a real step up that
  // only a career aimed at it closes, and not a fantasy.
  //
  // What that buys, measured over 300 rounds per rung:
  //   grade  9.2 -- tenth of thirteen. A rookie who is there and nowhere.
  //   grade  9.9 -- podium one round in five, win one in thirty.
  //   grade 10.6 -- podium four in five, win two in five.
  // Steep, because the odds curve is steep; the whole gap is a grade and a
  // half wide and every tenth of it shows.
  double worldStandard = 8.5;
  // Where the board sits relative to that standard -- an absolute grade,
  // not a bump on top of your own. Fed straight into
  // `CompDials::nationalGradeOffset` with `worldStandard` standing in for
  // your grade, so the number means exactly what it says.
  //
  // Coarse, like every grade dial that ends in `lround`: 1.0 and 1.5 set
  // the identical board. Move it by whole grades or not at all.
  double gradeBump = 1.0;
  // You have to be on the national team to be flown at all. The federation
  // pays for the plane; that is what the team is *for*.
  bool requiresTeam = true;

  // --- the Games -------------------------------------------------------
  // **Four years.** They come round on a cycle rather than a calendar,
  // and the cycle is the one everybody knows. At 56 days the probe entered
  // sixty Games in a ten-year career, which turns the biggest day in the
  // sport into a fortnightly errand -- an appearance is only worth
  // something because there are so few of them to have.
  //
  // A thirty-year career sees seven. Most see two or three.
  int olympicCycleDays = 1460;
  // You arrive before you compete, and the gap is where declaring happens.
  int olympicBuildUpDays = 3;
  // Never sooner than this from a standing start, so a save loaded the day
  // before the Games does not open onto them.
  int olympicSeedLead = 21;
  // **Deliberately the same as the World Cup's, and separate anyway.**
  //
  // The Games are above a World Cup in field and in what they are worth,
  // not in how hard the setting is -- every one of the seven is already at
  // or above the world standard, and a harder board on top of that would
  // make a medal arithmetically impossible rather than hard. Softer was
  // tried first and measured wrong the other way: at 0.5 a grade-9.9
  // climber medalled at 44% while podiuming a World Cup at 19%, because a
  // podium is three of eight here and three of thirteen there. Level
  // boards put it at 18% and 19%, which is the intended shape -- a medal
  // is as hard as a World Cup podium, and getting to the start line is
  // what makes it rarer.
  //
  // Kept as its own dial rather than folded into `gradeBump`: they are the
  // same number today and they are not the same decision.
  double olympicGradeBump = 1.0;
  // What it takes to be there at all: Olympic Hopeful.
  double qualifyAt = 1200.0;
};

// Lay out a season. Six venues, no venue twice, deterministic from the
// season number.
WorldCupSeason StartWorldCupSeason(const Rng& worldRng, int day, int season,
                                   const WorldStageDials& d = WorldStageDials{});

// Points for a placing. **1000 for a win and a steep drop after** — the
// real shape of an international table, where the difference between first
// and fourth is most of a season.
double WorldCupPoints(int place);

// Is a round today, and which?
int RoundToday(const WorldCupSeason& s, int day);   // -1 for none
int DaysUntilRound(const WorldCupSeason& s, int day,
                   const WorldStageDials& d = WorldStageDials{});

// The international field, twelve of them, spread around the world
// standard. **The offset is from the standard, not from you** -- see
// `WorldStageDials::worldStandard` for why that distinction is the whole
// design and why reading it the other way made the system unwinnable.
struct International {
  const char* name;
  const char* nation;
  double gradeOffset;
};
const std::vector<International>& TheWorldField();

// The comp dials a round is resolved under. **Same engine, different
// setting**: `nationalGradeOffset` becomes how far above your grade the
// board sits, and nothing else moves. Building a second resolver for the
// same verbs is how two comps start disagreeing about what a flash is
// worth.
CompDials WorldCupCompDials(const WorldStageDials& d = WorldStageDials{});
CompDials OlympicCompDials(const WorldStageDials& d = WorldStageDials{});

// Set a board. Five problems and seven goes, exactly like a Tuesday at the
// gym -- see the note at the top of this file for why that is on purpose.
//
// **Your grade is not an argument.** The board is where it is; what you can
// do on it is the question the round asks.
CompState SetTheWorldBoard(const Rng& worldRng, int day,
                           const WorldStageDials& d = WorldStageDials{});
CompState SetTheOlympicBoard(const Rng& worldRng, int day,
                             const WorldStageDials& d = WorldStageDials{});

// Rank you against the international field. This is the comp engine's own
// scoring -- `YourScore` and `CompetitorScore`, unchanged -- run against a
// different roster, because the roster is the only thing that differs. It
// is deliberately not `Settle`: `Settle` hard-codes the seven people you
// see at the gym, and having Kai win in Innsbruck would be the presentation
// telling a lie the sim did not.
CompResult SettleWorldRound(const CompState& comp, const Rng& compRng,
                            const WorldStageDials& d = WorldStageDials{});
CompResult SettleTheGames(const CompState& comp, const Rng& compRng,
                          const WorldStageDials& d = WorldStageDials{});

// Resolve a round. `yours` null means you were not there -- **the field
// still scores**, which is the whole point.
//
// When you flew, `yours->board` is authoritative: the season table is fed
// from the scorecard you actually watched, so the person who won the round
// is the person the table says won it. It comes from `SettleWorldRound`
// and it is not optional.
void BankRound(WorldCupSeason& s, int roundIndex, const CompResult* yours,
               const Rng& rng, const WorldStageDials& d = WorldStageDials{});

// **This season's** rounds, as opposed to the career counters above.
// `starts` and `missed` carry across seasons; these do not, and anything
// the player reads about the year they are in has to use these.
int RoundsFlown(const WorldCupSeason& s);
int RoundsMissed(const WorldCupSeason& s);

// The season table, best first.
std::vector<CircuitStanding> WorldTable(const WorldCupSeason& s);

// Has the last round been and gone?
bool WorldCupSeasonOver(const WorldCupSeason& s,
                        const WorldStageDials& d = WorldStageDials{});

// Close it out: your world ranking for the season, and a title if you took
// it. Returns your final place.
int CloseWorldCupSeason(WorldCupSeason& s);

// Can you get on the plane? **Everything the door has to check, in one
// answer** -- there is a round today, the federation is paying for you,
// and the ticket is affordable. Here rather than in the engine because a
// rule that lives in a UFUNCTION cannot be tested from the harness, and
// every rule this project has left in one has been found late.
struct FlightCheck {
  bool can = false;
  int round = -1;       // which round, -1 when there is none today
  double cost = 0.0;    // the ticket
  std::string why;      // why not, in the game's voice; empty when you can
};
FlightCheck CanFly(const WorldCupSeason& s, const NationalTeam& team,
                   double cash, int day,
                   const WorldStageDials& d = WorldStageDials{});

// What the season reads like.
std::string WorldCupLine(const WorldCupSeason& s, int day,
                         const WorldStageDials& d = WorldStageDials{});

// ---------------------------------------------------------------------
// The Games
// ---------------------------------------------------------------------

struct Olympics {
  int nextDay = 0;         // the day they are held; 0 = not seeded yet
  int appearances = 0;
  int gold = 0, silver = 0, bronze = 0;
  // Which cycle the last one you competed in was, so a Games cannot be
  // entered twice.
  int lastCompeted = -1;
};

// Seed the cycle. Deterministic from the world, and never sooner than the
// lead -- a save loaded the day before should not open onto the Games.
void SeedTheGames(Olympics& o, const Rng& worldRng, int today,
                  const WorldStageDials& d = WorldStageDials{});

// Roll the cycle forward once the day has passed.
void GamesDay(Olympics& o, int today,
              const WorldStageDials& d = WorldStageDials{});

// Are you good enough to be there?
bool Qualified(double rankingPoints,
               const WorldStageDials& d = WorldStageDials{});

// Are they on, and how far off?
bool GamesToday(const Olympics& o, int day);
int DaysUntilGames(const Olympics& o, int day);

// The Olympic field. Seven, and **every one of them is above you** -- which
// is what makes a medal worth something and an appearance worth having.
const std::vector<International>& TheOlympicField();

// What you came away with. `place` is 1-based.
struct Medal {
  int place = 0;
  bool gold = false, silver = false, bronze = false;
};
Medal MedalFor(int place);

// Record it.
void BankTheGames(Olympics& o, const Medal& m, int cycle);

// The same one-answer door as `CanFly`, for the same reason.
struct GamesCheck {
  bool can = false;
  std::string why;
};
GamesCheck CanEnterTheGames(const Olympics& o, double rankingPoints, int day,
                            const WorldStageDials& d = WorldStageDials{});

// What it reads like. Empty when there is nothing to say.
std::string GamesLine(const Olympics& o, double rankingPoints, int day,
                      const WorldStageDials& d = WorldStageDials{});

// ---------------------------------------------------------------------
// One night of it
// ---------------------------------------------------------------------

// Everything the world stage does while you sleep: open a season when
// there is none, bank the rounds the field flew and you did not, close a
// season when its last round has been and gone, start the next one after
// the off-season, and roll the Games.
//
// One function rather than five calls the engine has to remember to make.
// **Anything that counts down, counts down at night** -- three per-day
// rules in this project were written and left uncalled, and every one was
// found late.
struct WorldStageNight {
  bool seasonClosed = false;
  int place = 0;         // where you finished, when a season closed
  bool title = false;
  std::string news;      // empty unless there is something to say
};

// What a closed season reads like. **Rebuilt from state rather than only
// returned through the night tick**, because the engine's Sleep does not
// plumb a return value through the Blueprint library -- it reads the ledger
// across the call, the way every other piece of overnight news in this
// game already works. Empty when the season is not closed.
std::string WorldCupSeasonNews(const WorldCupSeason& s);
WorldStageNight WorldStageDay(WorldCupSeason& s, Olympics& o,
                              const Rng& worldRng, int day,
                              const WorldStageDials& d = WorldStageDials{});

}  // namespace dirtbag

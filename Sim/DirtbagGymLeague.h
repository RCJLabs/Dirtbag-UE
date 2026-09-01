// The league you run.
//
// **Not the league in `Sim/DirtbagLeague.h`.** That one is the low end of
// the comp system: you turn up on a Wednesday at somebody else's gym, pay
// five dollars, climb five problems and chase your own best score. This one
// you do not climb at all. You run it.
//
// `GYM-10`, ported from the 2D source, and its own comment is the argument:
// the existing leagues have covered Send City and The Cave since long
// before you owned anything, and `GYM-2`'s comp night is a one-off on a
// cooldown. **Neither is an institution.** A league is the same night every
// week, the same people, a table that carries, and at the end of a run
// somebody's name on it.
//
// ## The format is the whole decision, and it is not cosmetic
//
// It decides who wins. A board ladder belongs to whoever trains; a handicap
// league belongs to whoever improved; a team league belongs to whoever
// turns up with people; one go on one route belongs to whoever holds their
// nerve. **Every one of the thirteen regulars leans one way**, so the
// format you pick is a statement about who your gym is for, and the
// standings prove it week after week.
//
// It is also the first thing in the port that reads the cast `GYM-2` and
// `GYM-5` built for something other than a line of prose. A hardcore room
// running a team league has **nobody in it who suits the format** -- the
// cohort your set mix collected has no `Social` in it at all -- and that is
// not a bug, it is the two decisions disagreeing with each other in public,
// on a table on the wall, for six weeks.
//
// ## You are not in the standings, and the source's comment says you are
//
// `gymLeagueEntrants` is commented *"whichever regulars have a story with
// you, plus you"* and returns the regulars. The code is right and the
// comment is wrong, which is the third of those found in this family this
// week -- and the code is right for the same reason `GYM-9`'s host does not
// get a scorecard. **You run this.** Your name on the table would blur the
// one thing the table is for: you look at who is winning and you know what
// kind of night you built.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagGym.h"
#include "DirtbagGymFloor.h"

namespace dirtbag {

// Four nights, and each belongs to somebody different.
enum class LeagueFormat { Ladder, Handicap, Teams, Onesie, kLeagueFormatCount };
constexpr int kLeagueFormatCount =
    static_cast<int>(LeagueFormat::kLeagueFormatCount);

struct GymLeagueDials {
  // A run, then a champion, then a fresh table. Six weeks is short enough
  // that a bad run is over and long enough that one good night does not
  // decide it.
  int weeksInARun = 6;

  // Below this it is not a league, it is four people and a clipboard.
  double minMembers = 25.0;

  // Tape, prizes, and a printed table nobody will ever take down.
  double setupCost = 220.0;

  // **Named apart from `CompDials`' and `LeagueDials`' on purpose**, the
  // same convention `LeagueDials::nightFee` records: these are deliberately
  // different numbers from a comp's and from a league night you *climb*,
  // and the dial checker reads two dials of one name as one number that has
  // drifted. The names also say the thing that matters -- you are not in
  // this, you are running it.
  double runningItHours = 3.0;
  double runningItEnergy = 10.0;

  // Share of the membership who actually enter, and what each pays --
  // per format, because a one-go night is worth more at the door than a
  // handicap night and the table says which room you are running.
  double turnout = 0.35;
  double fee[kLeagueFormatCount] = {6.0, 5.0, 5.0, 8.0};

  // What running it does for your name, in scene points before the /100
  // the caller does. **The team league is worth the most and pays the
  // least**, which is the whole of its character.
  double standing[kLeagueFormatCount] = {2.0, 3.0, 4.0, 3.0};

  // --- and what a night is worth to a climber ---------------------------
  // A week's score is a base plus a spread plus, if the format suits them,
  // this.
  //
  // **Tuned, not guessed**, and the source records paying for it: its first
  // cut gave a suited climber +3.5 a week against a 3-10 base, which over
  // six weeks is +21 against a standings spread of about five -- so the
  // climbers a format suited won *every* run and only three people in the
  // building could ever hold the trophy. A format is supposed to tilt a
  // league, not decide it before it starts.
  double scoreBase = 3.0;
  double scoreSpread = 7.0;
  double suitsThem = 0.8;

  double psycheNight = 0.03;
  double psycheChampion = 0.06;
};

struct LeagueChampion {
  std::string name;
  int day = 0;
  int run = 0;
};

// Your league. `running` is what says there is one, the same shape `Gym`
// uses -- a save carries it either way and most careers do not have one.
struct GymLeague {
  bool running = false;
  std::string name;

  // 0 is Monday. Day 1 of a career is a Monday, the same as every other
  // weekday in this port.
  int night = 2;

  LeagueFormat format = LeagueFormat::Ladder;

  // Week within the current run, and how many runs have finished.
  int week = 0;
  int runs = 0;

  // The table, indexed by `GymRegular`. Cleared when a run ends, because a
  // table that carried forever would be a career ranking and this is a
  // season.
  double points[kGymRegularCount] = {0.0};

  int lastNightDay = -99;
  std::vector<LeagueChampion> champions;
};

const char* LeagueFormatName(LeagueFormat f);
const char* LeagueFormatBlurb(LeagueFormat f);

// Which leaning a format rewards. **This is the load-bearing table**: it is
// why picking a format is picking a kind of person.
Leaning FormatFavours(LeagueFormat f);

// Monday to Sunday, 0-based.
const char* NightName(int night);
int NightOf(int day);

bool ItIsLeagueNight(const GymLeague& league, int day);

// Why you cannot start one, or empty.
std::string WhyNotStartALeague(const Gym& gym, const GymLeague& league,
                               double cash,
                               const GymLeagueDials& dials = GymLeagueDials{});

// Start it. The night and the format are the whole decision; the name comes
// off the gym and the format, because a league is called what it is.
bool StartALeague(GymLeague& league, const Gym& gym, double& cash,
                  LeagueFormat format, int night,
                  const GymLeagueDials& dials = GymLeagueDials{});

// Why not tonight, or empty. A different sentence for each of the three
// reasons, because the player can only do something about one of them.
std::string WhyNotTonight(const GymLeague& league, int day, double energy,
                          const GymLeagueDials& dials = GymLeagueDials{});

struct LeagueNightRan {
  bool ran = false;
  std::string said;
  // Handed back rather than added, because entry money is income and this
  // game pays its debts first.
  double takings = 0.0;
  double standing = 0.0;
  double psyche = 0.0;
  int week = 0;
  // Who is top of the table tonight, and -- on the last week of a run --
  // whose name goes on it.
  GymRegular leader = GymRegular::None;
  GymRegular champion = GymRegular::None;
};

// Run a week. **Scoring is derived from the league's name, the week and the
// climber**, so a table can be rebuilt rather than trusted and no two
// readings of the same night disagree.
LeagueNightRan RunLeagueNight(GymLeague& league, const Gym& gym,
                              const GymFloor& floor, int day,
                              const GymLeagueDials& dials = GymLeagueDials{});

// Who is in the room: whichever regulars have a story with you. Empty until
// somebody's arc has started, which is what makes the league something the
// floor earns rather than something the building comes with.
std::vector<GymRegular> WhoTurnsUp(const GymFloor& floor);

struct LeagueStanding {
  GymRegular who = GymRegular::None;
  std::string name;
  double points = 0.0;
  bool suited = false;
};

// The table as it stands, best first.
std::vector<LeagueStanding> TheTable(const GymLeague& league,
                                     const GymFloor& floor);

// One line for the wall: the night, the week, and who is leading.
std::string GymLeagueLine(const GymLeague& league, const GymFloor& floor,
                          int day,
                          const GymLeagueDials& dials = GymLeagueDials{});

}  // namespace dirtbag

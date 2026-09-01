#include "DirtbagGymLeague.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

int Slot(LeagueFormat f) { return static_cast<int>(f); }
// Named apart from `DirtbagGymFloor`'s: the unity build puts every
// anonymous namespace in one, and two files may not both define `Slot` for
// the same type.
int Seat(GymRegular who) { return static_cast<int>(who); }

double LeagueDraw(const std::string& label) {
  return Rng::FromSeed(label).NextDouble();
}

const char* kNights[] = {"Monday",   "Tuesday", "Wednesday", "Thursday",
                         "Friday",   "Saturday", "Sunday"};
constexpr int kNightCount = static_cast<int>(sizeof(kNights) / sizeof(kNights[0]));

}  // namespace

const char* LeagueFormatName(LeagueFormat f) {
  switch (f) {
    case LeagueFormat::Ladder: return "Board Ladder";
    case LeagueFormat::Handicap: return "Handicap";
    case LeagueFormat::Teams: return "Teams of Three";
    case LeagueFormat::Onesie: return "One Go, One Route";
    default: return "";
  }
}

const char* LeagueFormatBlurb(LeagueFormat f) {
  switch (f) {
    case LeagueFormat::Ladder:
      return "Hardest thing you can do on the board, best two of five "
             "weeks. No excuses in it anywhere.";
    case LeagueFormat::Handicap:
      return "You are scored against your own best, not against the room. "
             "The strongest person here can lose.";
    case LeagueFormat::Teams:
      return "Drawn out of a hat every week. Half of it is climbing and "
             "half of it is who you got.";
    case LeagueFormat::Onesie:
      return "One route, one attempt, everybody watching. Over in ninety "
             "seconds and talked about for a month.";
    default: return "";
  }
}

Leaning FormatFavours(LeagueFormat f) {
  switch (f) {
    case LeagueFormat::Ladder: return Leaning::Power;
    case LeagueFormat::Handicap: return Leaning::Improve;
    case LeagueFormat::Teams: return Leaning::Social;
    case LeagueFormat::Onesie: return Leaning::Nerve;
    default: return Leaning::Social;
  }
}

const char* NightName(int night) {
  return night >= 0 && night < kNightCount ? kNights[night] : "";
}

int NightOf(int day) {
  // Day 1 is a Monday, the same as `SalariedToday` reads it.
  return ((day - 1) % kNightCount + kNightCount) % kNightCount;
}

bool ItIsLeagueNight(const GymLeague& league, int day) {
  return league.running && NightOf(day) == league.night;
}

std::string WhyNotStartALeague(const Gym& gym, const GymLeague& league,
                               double cash, const GymLeagueDials& dials) {
  if (!gym.owned) return "";
  if (league.running) return "";
  if (gym.members < dials.minMembers) {
    return std::to_string(static_cast<int>(dials.minMembers)) +
           " members before it is a league rather than four people and a "
           "clipboard. You have " +
           std::to_string(static_cast<int>(gym.members)) + ".";
  }
  if (cash < dials.setupCost) {
    return "Tape, prizes and a printed table run $" +
           std::to_string(static_cast<int>(dials.setupCost)) + ".";
  }
  return "";
}

bool StartALeague(GymLeague& league, const Gym& gym, double& cash,
                  LeagueFormat format, int night,
                  const GymLeagueDials& dials) {
  if (!gym.owned || league.running) return false;
  if (format == LeagueFormat::kLeagueFormatCount) return false;
  if (night < 0 || night >= kNightCount) return false;
  if (!WhyNotStartALeague(gym, league, cash, dials).empty()) return false;

  cash -= dials.setupCost;
  league = GymLeague{};
  league.running = true;
  league.format = format;
  league.night = night;
  league.name = gym.name + " " + LeagueFormatName(format);
  league.lastNightDay = -99;
  return true;
}

std::string WhyNotTonight(const GymLeague& league, int day, double energy,
                          const GymLeagueDials& dials) {
  if (!league.running) return "";
  if (!ItIsLeagueNight(league, day)) {
    return league.name + " runs on " + NightName(league.night) + "s.";
  }
  if (league.lastNightDay == day) {
    return "Tonight is done. The table is on the wall.";
  }
  if (energy < dials.runningItEnergy) return "Too wrecked to run it tonight.";
  return "";
}

std::vector<GymRegular> WhoTurnsUp(const GymFloor& floor) {
  std::vector<GymRegular> out;
  for (GymRegular who : TheCast(floor)) {
    // **A story with you, however short.** One stage lived is enough --
    // somebody you have never spoken to is a membership, not an entrant.
    if (floor.stage[Seat(who)] >= 1) out.push_back(who);
  }
  return out;
}

LeagueNightRan RunLeagueNight(GymLeague& league, const Gym& gym,
                              const GymFloor& floor, int day,
                              const GymLeagueDials& dials) {
  LeagueNightRan out;
  if (!gym.owned || !league.running) return out;
  // The night's own gates, so a caller cannot run two Wednesdays on one
  // Wednesday. Energy is the caller's to check -- it is the only one of the
  // three the league itself does not know about.
  if (!ItIsLeagueNight(league, day)) return out;
  if (league.lastNightDay == day) return out;

  const int week = league.week + 1;
  const Leaning favours = FormatFavours(league.format);
  const std::vector<GymRegular> here = WhoTurnsUp(floor);

  for (GymRegular who : here) {
    const GymRegularDef* def = GymRegularOf(who);
    if (def == nullptr) continue;
    // **The run number is in the seed, and the source's is not.**
    //
    // Its key is the league's name, the week and the climber -- and the
    // week resets to one when a run ends, so every run of a league scores
    // *identically*: the same six nights, the same table, the same winner,
    // forever. Measured before this line: over 251 runs of one league,
    // **two different names ever went on the wall and only one of them
    // suited the format.**
    //
    // That is the exact failure the source records tuning `suitsThem` away
    // from -- "only three people in the building could ever hold the
    // trophy" -- arriving through a door the tuning could not see, because
    // a champion distribution measured across *different* leagues looks
    // fine while every run of *one* league is a rerun.
    const double roll =
        LeagueDraw("gymleague|" + league.name + "|" +
                   std::to_string(league.runs) + "|" + std::to_string(week) +
                   "|" + def->name);
    const double scored = dials.scoreBase + roll * dials.scoreSpread +
                          (def->lean == favours ? dials.suitsThem : 0.0);
    // A tenth of a point, so the table reads like a table.
    league.points[Seat(who)] =
        std::round((league.points[Seat(who)] + scored) * 10.0) / 10.0;
  }

  // Whoever is top tonight, and cast order breaks a tie -- the same rule
  // the floor walk uses, so two readouts never disagree about who leads.
  for (GymRegular who : here) {
    if (out.leader == GymRegular::None ||
        league.points[Seat(who)] > league.points[Seat(out.leader)]) {
      out.leader = who;
    }
  }

  out.ran = true;
  out.week = week;
  out.takings = std::round(gym.members * dials.turnout) *
                dials.fee[Slot(league.format)];
  out.standing = dials.standing[Slot(league.format)];
  league.lastNightDay = day;

  const bool runEnds = week >= dials.weeksInARun;
  if (runEnds) {
    // **The run ends, somebody's name goes on it, and the table starts
    // again at nothing.** That ending is what stops a weekly event from
    // being wallpaper.
    out.champion = out.leader;
    league.runs++;
    league.week = 0;
    for (int i = 0; i < kGymRegularCount; i++) league.points[i] = 0.0;
    out.psyche = dials.psycheChampion;
  } else {
    league.week = week;
    out.psyche = dials.psycheNight;
  }

  const GymRegularDef* lead = GymRegularOf(out.leader);
  if (out.champion != GymRegular::None && lead != nullptr) {
    league.champions.push_back(
        LeagueChampion{lead->name, day, league.runs});
    out.said = std::string("Final week of the ") + league.name + ". " +
               lead->name + " takes it. The table comes down, a new one goes "
               "up.";
  } else if (lead != nullptr) {
    out.said = league.name + ", week " + std::to_string(week) + " of " +
               std::to_string(dials.weeksInARun) + ". " + lead->name +
               " leads the table.";
  } else {
    // Nobody has a story with you yet, so nobody entered. The night still
    // happened and the door still took money; there is simply no table.
    out.said = league.name + ", week " + std::to_string(week) +
               ". Nobody you know was in it.";
  }
  return out;
}

std::vector<LeagueStanding> TheTable(const GymLeague& league,
                                     const GymFloor& floor) {
  std::vector<LeagueStanding> table;
  if (!league.running) return table;
  const Leaning favours = FormatFavours(league.format);
  for (GymRegular who : WhoTurnsUp(floor)) {
    const GymRegularDef* def = GymRegularOf(who);
    if (def == nullptr) continue;
    LeagueStanding row;
    row.who = who;
    row.name = def->name;
    row.points = league.points[Seat(who)];
    row.suited = def->lean == favours;
    table.push_back(row);
  }
  std::stable_sort(table.begin(), table.end(),
                   [](const LeagueStanding& a, const LeagueStanding& b) {
                     return a.points > b.points;
                   });
  return table;
}

std::string GymLeagueLine(const GymLeague& league, const GymFloor& floor,
                          int day, const GymLeagueDials& dials) {
  if (!league.running) return "";
  std::string out = league.name + ", " + NightName(league.night) + "s";
  out += "  --  week " + std::to_string(league.week + 1) + " of " +
         std::to_string(dials.weeksInARun);
  const std::vector<LeagueStanding> table = TheTable(league, floor);
  if (!table.empty()) {
    out += ", " + table.front().name + " leading";
  } else {
    // **The honest empty state.** A format nobody in the room suits still
    // runs; a room with nobody in it you know does not have a table.
    out += ", nobody you know in it yet";
  }
  if (!league.champions.empty()) {
    out += "  --  " + std::to_string(league.champions.size()) +
           (league.champions.size() == 1 ? " name on the wall"
                                         : " names on the wall");
  }
  if (ItIsLeagueNight(league, day) && league.lastNightDay != day) {
    out += "  --  tonight";
  }
  return out;
}

}  // namespace dirtbag

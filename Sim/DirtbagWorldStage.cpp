#include "DirtbagWorldStage.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

// The real shape of an international table: **1000 for a win and a cliff
// after it.** The difference between first and fourth is most of a season,
// which is why a World Cup year turns on two or three rounds.
const double kPoints[30] = {1000, 805, 690, 610, 545, 495, 455, 415, 380, 350,
                            325,  300, 280, 260, 240, 225, 210, 195, 185, 175,
                            165,  155, 145, 140, 135, 130, 125, 120, 115, 110};

}  // namespace

const std::vector<WorldCupVenue>& TheVenues() {
  static const std::vector<WorldCupVenue> kVenues = {
      {"Salt Lake City", "USA", Discipline::Boulder, 210.0,
       "The cheap one. An airport you can reach on a domestic ticket and a "
       "crowd that actually knows what a heel hook is."},
      {"Innsbruck", "Austria", Discipline::Sport, 610.0,
       "The one everybody wants. A wall in a valley with mountains behind "
       "it, and the loudest lead crowd on earth."},
      {"Villars", "Switzerland", Discipline::Sport, 640.0,
       "Outdoor wall, alpine village, and a real chance the whole thing "
       "runs two hours late because of weather."},
      {"Briancon", "France", Discipline::Sport, 600.0,
       "Finals under lights at altitude, in July, at nine at night, and it "
       "is still warm. Nobody sleeps after."},
      {"Chamonix", "France", Discipline::Sport, 620.0,
       "You climb with the Aiguilles over your shoulder and every third "
       "person in the crowd is an alpinist who thinks this is silly."},
      {"Bern", "Switzerland", Discipline::Boulder, 630.0,
       "A tidy hall, a tidy country, and a boulder round that will be "
       "decided by one slab nobody can read."},
      {"Seoul", "Korea", Discipline::Boulder, 790.0,
       "Long haul, big hall, and the deepest home team in bouldering. You "
       "will get beaten by somebody you have never heard of."},
      {"Koper", "Slovenia", Discipline::Sport, 650.0,
       "Sea on one side, a lead wall on the other, and a small country that "
       "produces an unreasonable number of the world's best."},
      {"Wujiang", "China", Discipline::Boulder, 800.0,
       "Two lanes, five seconds, and a false start that ends your day "
       "before it starts."},
      {"Prague", "Czechia", Discipline::Boulder, 640.0,
       "A round in a city square with the wall bolted to scaffolding and a "
       "beer tent doing better numbers than the ticket office."},
  };
  return kVenues;
}

const std::vector<International>& TheWorldField() {
  // Twelve, spread a grade and a half either side of the world standard --
  // **offsets from the standard, not from you.** This is their actual job,
  // where the domestic field is seven people you see at the gym, and it is
  // why a maxed-out climber is mid-table here rather than at the front.
  static const std::vector<International> kField = {
      {"Sora", "JPN", 1.5},       {"Adler", "GER", 1.4},
      {"Lindqvist", "SWE", 1.2},  {"Moreau", "FRA", 1.1},
      {"Okafor", "GBR", 0.9},     {"Cortez", "ESP", 0.8},
      {"Volkov", "CAN", 0.5},     {"Haas", "AUT", 0.4},
      {"Park", "KOR", 0.1},       {"Bianchi", "ITA", 0.0},
      {"Novak", "SLO", -0.4},     {"Rensch", "NED", -0.8},
  };
  return kField;
}

const std::vector<International>& TheOlympicField() {
  // Seven, and **every one of them is at or above the standard** -- the top
  // of the World Cup roster and nobody else. That is what makes a medal
  // worth something and an appearance worth having on its own.
  static const std::vector<International> kField = {
      {"Sora", "JPN", 1.5},      {"Adler", "GER", 1.4},
      {"Lindqvist", "SWE", 1.2}, {"Moreau", "FRA", 1.1},
      {"Okafor", "GBR", 0.9},    {"Volkov", "CAN", 0.5},
      {"Reyes", "ESP", 0.3},
  };
  return kField;
}

double WorldCupPoints(int place) {
  if (place <= 0) return 0.0;
  const int i = place - 1;
  // Off the end of the table still scores. Turning up to a World Cup and
  // coming thirty-fifth is a thing that happens to real climbers and it is
  // not worth nothing.
  return i < 30 ? kPoints[i] : 10.0;
}

WorldCupSeason StartWorldCupSeason(const Rng& worldRng, int day, int season,
                                   const WorldStageDials& d) {
  WorldCupSeason s;
  s.season = season;
  s.fieldPoints.assign(TheWorldField().size(), 0.0);
  Rng rng = worldRng.Derive("wc#" + std::to_string(season));

  // Six venues, **no venue twice** -- a season that visits Innsbruck three
  // times is a schedule nobody wrote.
  std::vector<int> pool;
  for (std::size_t i = 0; i < TheVenues().size(); i++) {
    pool.push_back(static_cast<int>(i));
  }
  for (int i = static_cast<int>(pool.size()) - 1; i > 0; i--) {
    std::swap(pool[i], pool[rng.IntRange(0, i)]);
  }

  int at = day + d.publishedDaysAhead;
  for (int i = 0; i < d.rounds && i < static_cast<int>(pool.size()); i++) {
    WorldCupRound r;
    r.day = at;
    r.venue = pool[i];
    s.schedule.push_back(r);
    at += d.roundGapMin +
          rng.IntRange(0, std::max(0, d.roundGapVariance - 1));
  }
  return s;
}

int RoundToday(const WorldCupSeason& s, int day) {
  for (std::size_t i = 0; i < s.schedule.size(); i++) {
    if (s.schedule[i].day == day && !s.schedule[i].resolved) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int DaysUntilRound(const WorldCupSeason& s, int day,
                   const WorldStageDials& d) {
  for (const WorldCupRound& r : s.schedule) {
    if (r.resolved || r.day < day) continue;
    const int until = r.day - day;
    return until <= d.publishedDaysAhead ? until : -1;
  }
  return -1;
}

CompDials WorldCupCompDials(const WorldStageDials& d) {
  CompDials cd;
  // The one thing that moves. `SetTheBoard` prices a National board at
  // `yourGrade + nationalGradeOffset`, so feeding the bump in here means
  // the dial says exactly how far above you the board sits and nothing is
  // added to it downstream.
  cd.nationalGradeOffset = d.gradeBump;
  return cd;
}

CompDials OlympicCompDials(const WorldStageDials& d) {
  CompDials cd;
  cd.nationalGradeOffset = d.olympicGradeBump;
  return cd;
}

CompState SetTheWorldBoard(const Rng& worldRng, int day,
                           const WorldStageDials& d) {
  // `worldStandard` stands where a domestic comp puts your grade. That
  // substitution is the whole of what "absolute rather than relative"
  // means in code.
  return SetTheBoard(worldRng.Derive("world-cup"), CompTier::National,
                     d.worldStandard, day, WorldCupCompDials(d));
}

CompState SetTheOlympicBoard(const Rng& worldRng, int day,
                             const WorldStageDials& d) {
  return SetTheBoard(worldRng.Derive("the-games"), CompTier::National,
                     d.worldStandard, day, OlympicCompDials(d));
}

namespace {

// The shared half of both settles: your score, their scores, one sort.
// The roster and the dials are the arguments because they are the only
// things that differ between a World Cup round and a Games final.
CompResult SettleAgainst(const CompState& comp, double standard,
                         const std::vector<International>& field,
                         const Rng& compRng, const CompDials& cd) {
  CompResult out;
  out.yourScore = YourScore(comp, cd);
  out.board.push_back(CompEntrant{"You", out.yourScore, true, false});

  Rng rng = compRng.Derive("world-field");
  for (const International& c : field) {
    CompEntrant e;
    e.name = std::string(c.name) + " (" + c.nation + ")";
    // Signature and weakness are deliberately flat here. At this level
    // everybody can climb everything; what separates them is the grade
    // they carry and the day they are having, which is exactly what the
    // domestic field's offsets and form roll already model.
    e.score = CompetitorScore(comp.problems, standard + c.gradeOffset,
                              RouteType::Power, RouteType::Power,
                              cd.formLow + rng.NextDouble() * cd.formRange,
                              cd);
    out.board.push_back(e);
  }

  std::stable_sort(out.board.begin(), out.board.end(),
                   [](const CompEntrant& a, const CompEntrant& b) {
                     if (a.score != b.score) return a.score > b.score;
                     // The same rule the domestic board learned the hard
                     // way: **a tie at zero is not a tie**, it is a room
                     // full of people who did not climb, and you do not get
                     // to take it.
                     if (a.score <= 0.0) return !a.isYou && b.isYou;
                     return a.isYou && !b.isYou;
                   });

  out.fieldSize = static_cast<int>(out.board.size());
  for (std::size_t i = 0; i < out.board.size(); i++) {
    if (out.board[i].isYou) out.place = static_cast<int>(i) + 1;
  }
  // No prize money and no scene rep. **The World Cup pays in ranking and
  // nothing else** -- the federation flew you, the podium gets a cheque
  // that goes to the federation, and what you take home is the table.
  return out;
}

}  // namespace

CompResult SettleWorldRound(const CompState& comp, const Rng& compRng,
                            const WorldStageDials& d) {
  return SettleAgainst(comp, d.worldStandard, TheWorldField(), compRng,
                       WorldCupCompDials(d));
}

CompResult SettleTheGames(const CompState& comp, const Rng& compRng,
                          const WorldStageDials& d) {
  return SettleAgainst(comp, d.worldStandard, TheOlympicField(), compRng,
                       OlympicCompDials(d));
}

void BankRound(WorldCupSeason& s, int roundIndex, const CompResult* yours,
               const Rng& rng, const WorldStageDials& d) {
  if (roundIndex < 0 || roundIndex >= static_cast<int>(s.schedule.size())) {
    return;
  }
  WorldCupRound& r = s.schedule[roundIndex];
  if (r.resolved) return;

  const std::vector<International>& field = TheWorldField();
  if (s.fieldPoints.size() != field.size()) {
    s.fieldPoints.assign(field.size(), 0.0);
  }

  // **The field flies whether you do or not.** This is the whole mechanic
  // and it is the reason a skipped round is not a round that did not
  // happen: they are banking, and the table moves away from you while you
  // are at home. Ranked by grade with form on the day, exactly like a
  // domestic comp -- the *people* are harder, not the maths.
  if (yours != nullptr) {
    r.flown = true;
    s.starts++;
    // **The scorecard is the table.** Points come off the board you
    // actually watched, matched back to the field by name, so the climber
    // the round says won it is the climber the season says won it. The
    // first version banked from a second, independent form roll -- which
    // meant the person who beat you on the day and the person leading the
    // season had nothing to do with each other.
    for (std::size_t i = 0; i < yours->board.size(); i++) {
      const int place = static_cast<int>(i) + 1;
      const CompEntrant& e = yours->board[i];
      if (e.isYou) {
        s.yourPoints += WorldCupPoints(place);
        if (place <= 6) s.finals++;
        if (place <= 3) s.podiums++;
        if (place == 1) s.wins++;
        continue;
      }
      for (std::size_t f = 0; f < field.size(); f++) {
        if (e.name.compare(0, std::string(field[f].name).size(),
                           field[f].name) == 0) {
          s.fieldPoints[f] += WorldCupPoints(place);
          break;
        }
      }
    }
  } else {
    r.flown = false;
    s.missed++;
    // Nobody watched this one, so there is no scorecard to read -- the
    // round is resolved by the grades they carry and the day they had,
    // which is the same maths `SettleAgainst` runs with the board taken
    // out. That is the whole of what "the field flies whether you do or
    // not" costs to implement, and it is the whole of what it means.
    Rng form = rng.Derive("wc-form#" + std::to_string(roundIndex));
    std::vector<std::pair<double, int>> order;   // score, field index
    for (std::size_t i = 0; i < field.size(); i++) {
      const double f = 0.79 + form.NextDouble() * 0.42;
      order.push_back({(field[i].gradeOffset + 10.0) * f,
                       static_cast<int>(i)});
    }
    std::sort(order.begin(), order.end(),
              [](const std::pair<double, int>& a,
                 const std::pair<double, int>& b) {
                return a.first > b.first;
              });
    for (std::size_t i = 0; i < order.size(); i++) {
      s.fieldPoints[order[i].second] +=
          WorldCupPoints(static_cast<int>(i) + 1);
    }
  }
  r.resolved = true;
  (void)d;
}

int RoundsFlown(const WorldCupSeason& s) {
  int n = 0;
  for (const WorldCupRound& r : s.schedule) {
    if (r.resolved && r.flown) n++;
  }
  return n;
}

int RoundsMissed(const WorldCupSeason& s) {
  int n = 0;
  for (const WorldCupRound& r : s.schedule) {
    if (r.resolved && !r.flown) n++;
  }
  return n;
}

std::vector<CircuitStanding> WorldTable(const WorldCupSeason& s) {
  std::vector<CircuitStanding> table;
  table.push_back(CircuitStanding{"You", s.yourPoints, true, false});
  const std::vector<International>& field = TheWorldField();
  for (std::size_t i = 0; i < field.size() && i < s.fieldPoints.size(); i++) {
    table.push_back(CircuitStanding{
        std::string(field[i].name) + " (" + field[i].nation + ")",
        s.fieldPoints[i], false, false});
  }
  std::stable_sort(table.begin(), table.end(),
                   [](const CircuitStanding& a, const CircuitStanding& b) {
                     if (a.points != b.points) return a.points > b.points;
                     if (a.points <= 0.0) return !a.isYou && b.isYou;
                     return a.isYou && !b.isYou;
                   });
  return table;
}

bool WorldCupSeasonOver(const WorldCupSeason& s, const WorldStageDials& d) {
  if (s.season <= 0 || s.schedule.empty()) return false;
  for (const WorldCupRound& r : s.schedule) {
    if (!r.resolved) return false;
  }
  (void)d;
  return true;
}

FlightCheck CanFly(const WorldCupSeason& s, const NationalTeam& team,
                   double cash, int day, const WorldStageDials& d) {
  FlightCheck out;
  out.round = RoundToday(s, day);
  if (out.round < 0) return out;   // nothing on; nothing to say
  out.cost = TheVenues()[s.schedule[out.round].venue].travel;

  // **The federation pays for the plane. That is what the team is for.**
  // Without this the World Cup is a shop you buy placings from, and the
  // whole domestic ladder underneath it stops being the way up.
  if (d.requiresTeam && team.status != TeamStatus::Named) {
    out.why = "World Cup entries go through the federation. You are not on "
              "the team.";
    return out;
  }
  if (cash < out.cost) {
    out.why = "The federation covers the entry. It does not cover the "
              "flight.";
    return out;
  }
  out.can = true;
  return out;
}

GamesCheck CanEnterTheGames(const Olympics& o, double rankingPoints, int day,
                            const WorldStageDials& d) {
  GamesCheck out;
  if (!GamesToday(o, day)) return out;
  if (o.lastCompeted == o.nextDay) {
    out.why = "You have had your day.";
    return out;
  }
  if (!Qualified(rankingPoints, d)) {
    // Said as a number, because it is the one thing on this ladder a
    // player can do something about.
    out.why = "You are not qualified. That is " +
              std::to_string(
                  static_cast<int>(d.qualifyAt - rankingPoints)) +
              " ranking points away.";
    return out;
  }
  out.can = true;
  return out;
}

int CloseWorldCupSeason(WorldCupSeason& s) {
  const std::vector<CircuitStanding> table = WorldTable(s);
  int place = 0;
  for (std::size_t i = 0; i < table.size(); i++) {
    if (table[i].isYou) place = static_cast<int>(i) + 1;
  }
  s.lastRank = place;
  s.closed = true;
  if (place == 1 && s.yourPoints > 0.0) s.titles++;
  if (place > 0 && (s.bestRank == 0 || place < s.bestRank)) {
    s.bestRank = place;
  }
  return place;
}

std::string WorldCupLine(const WorldCupSeason& s, int day,
                         const WorldStageDials& d) {
  if (s.season <= 0) return std::string();
  const int next = DaysUntilRound(s, day, d);
  if (next == 0) {
    for (const WorldCupRound& r : s.schedule) {
      if (r.day == day && !r.resolved) {
        return std::string("World Cup: ") + TheVenues()[r.venue].city +
               " is today.";
      }
    }
  }
  if (next > 0) {
    for (const WorldCupRound& r : s.schedule) {
      if (!r.resolved && r.day == day + next) {
        return std::string("World Cup: ") + TheVenues()[r.venue].city +
               (next == 1 ? " tomorrow." : " this week.") + "  $" +
               std::to_string(static_cast<int>(TheVenues()[r.venue].travel)) +
               " to get there.";
      }
    }
  }
  // Between rounds: where you are, and **what it cost you to stay home**.
  const std::vector<CircuitStanding> table = WorldTable(s);
  int place = 0;
  for (std::size_t i = 0; i < table.size(); i++) {
    if (table[i].isYou) place = static_cast<int>(i) + 1;
  }
  std::string out = "World Cup season " + std::to_string(s.season) + ": " +
                    std::to_string(place) + " of " +
                    std::to_string(static_cast<int>(table.size()));
  // **This season's**, counted off the schedule. `s.missed` is a career
  // total and carries across seasons, so reading it here would tell a
  // climber in their fourth year that they had missed twenty rounds of a
  // six-round season.
  const int missed = RoundsMissed(s);
  if (missed > 0) {
    out += ", " + std::to_string(missed) +
           (missed == 1 ? " round missed." : " rounds missed.");
  } else {
    out += ".";
  }
  return out;
}

// ---------------------------------------------------------------------

void SeedTheGames(Olympics& o, const Rng& worldRng, int today,
                  const WorldStageDials& d) {
  Rng rng = worldRng.Derive("the-games");
  // Never sooner than the lead. A save loaded the day before should not
  // open onto the Games, and a fresh career should not either.
  o.nextDay = today + d.olympicSeedLead +
              rng.IntRange(0, std::max(1, d.olympicCycleDays - 1));
}

void GamesDay(Olympics& o, int today, const WorldStageDials& d) {
  if (o.nextDay <= 0) return;
  // Rolled forward the day *after*, so the day itself stays enterable for
  // its whole length.
  while (today > o.nextDay) o.nextDay += d.olympicCycleDays;
}

bool Qualified(double rankingPoints, const WorldStageDials& d) {
  return rankingPoints >= d.qualifyAt;
}

bool GamesToday(const Olympics& o, int day) {
  return o.nextDay > 0 && o.nextDay == day;
}

int DaysUntilGames(const Olympics& o, int day) {
  if (o.nextDay <= 0 || o.nextDay < day) return -1;
  return o.nextDay - day;
}

Medal MedalFor(int place) {
  Medal m;
  m.place = place;
  m.gold = place == 1;
  m.silver = place == 2;
  m.bronze = place == 3;
  return m;
}

void BankTheGames(Olympics& o, const Medal& m, int cycle) {
  if (o.lastCompeted == cycle) return;   // one Games per cycle
  o.lastCompeted = cycle;
  o.appearances++;
  if (m.gold) o.gold++;
  else if (m.silver) o.silver++;
  else if (m.bronze) o.bronze++;
}

std::string GamesLine(const Olympics& o, double rankingPoints, int day,
                      const WorldStageDials& d) {
  const int until = DaysUntilGames(o, day);
  const bool qualified = Qualified(rankingPoints, d);

  if (until == 0) {
    return qualified ? "The Games are today."
                     : "The Games are today. You are watching them.";
  }
  // **Silent unless it is close or you have been.** A countdown to
  // something you are eight hundred points away from is a progress bar for
  // a system the player has not met.
  if (until > 0 && until <= d.olympicBuildUpDays && qualified) {
    return until == 1 ? "The Games are tomorrow."
                      : "The Games are in a few days.";
  }
  if (o.appearances > 0) {
    std::string s = std::to_string(o.appearances) +
                    (o.appearances == 1 ? " Games." : " Games.");
    if (o.gold + o.silver + o.bronze > 0) {
      s += " ";
      if (o.gold > 0) s += std::to_string(o.gold) + " gold ";
      if (o.silver > 0) s += std::to_string(o.silver) + " silver ";
      if (o.bronze > 0) s += std::to_string(o.bronze) + " bronze";
    }
    return s;
  }
  return std::string();
}

std::string WorldCupSeasonNews(const WorldCupSeason& s) {
  if (!s.closed || s.season <= 0) return std::string();
  std::string news = "World Cup season " + std::to_string(s.season) +
                     " is done. You finished " + std::to_string(s.lastRank) +
                     " of " +
                     std::to_string(static_cast<int>(WorldTable(s).size()));
  if (RoundsFlown(s) == 0) {
    // Said plainly rather than hidden. Finishing last of thirteen because
    // you never got on a plane is a different sentence from finishing last
    // because you did.
    news += ", without leaving the country.";
  } else {
    news += ".";
  }
  if (s.lastRank == 1 && s.yourPoints > 0.0) {
    news += "  That is a World Cup title.";
  }
  return news;
}

// ---------------------------------------------------------------------

WorldStageNight WorldStageDay(WorldCupSeason& s, Olympics& o,
                              const Rng& worldRng, int day,
                              const WorldStageDials& d) {
  WorldStageNight night;

  // The Games first, because seeding them is what a career that has never
  // heard of them needs and rolling them is what one that has needs.
  if (o.nextDay <= 0) {
    SeedTheGames(o, worldRng, day, d);
  } else {
    GamesDay(o, day, d);
  }

  // **A season runs whether you are in it or not.** Same rule the domestic
  // circuit lives under and the same reason: a scene that waits for you is
  // not a scene.
  if (s.season <= 0) {
    s = StartWorldCupSeason(worldRng, day, 1, d);
    return night;
  }

  // A round in the past that nobody flew to is a round the field flew to.
  // Banked here rather than at the door, because the door is only opened
  // by a player who turned up.
  for (std::size_t i = 0; i < s.schedule.size(); i++) {
    if (s.schedule[i].resolved || s.schedule[i].day >= day) continue;
    BankRound(s, static_cast<int>(i),
              nullptr,
              worldRng.Derive("wc-miss#" + std::to_string(s.season) + "#" +
                              std::to_string(i)),
              d);
  }

  if (WorldCupSeasonOver(s, d)) {
    if (!s.closed) {
      const int titlesBefore = s.titles;
      night.place = CloseWorldCupSeason(s);
      night.seasonClosed = true;
      night.title = s.titles > titlesBefore;
      night.news = WorldCupSeasonNews(s);
    } else {
      const int last = s.schedule.empty() ? day : s.schedule.back().day;
      if (day - last >= d.worldSeasonBreakDays) {
        const int season = s.season + 1;
        const int titles = s.titles, best = s.bestRank, lastRank = s.lastRank;
        const int starts = s.starts, missed = s.missed, finals = s.finals;
        const int podiums = s.podiums, wins = s.wins;
        s = StartWorldCupSeason(worldRng, day, season, d);
        // The career carries over; the season does not. Same split the
        // domestic circuit makes, for the same reason -- a table resets and
        // a record does not.
        s.titles = titles;
        s.bestRank = best;
        s.lastRank = lastRank;
        s.starts = starts;
        s.missed = missed;
        s.finals = finals;
        s.podiums = podiums;
        s.wins = wins;
      }
    }
  }
  return night;
}

}  // namespace dirtbag

#include "DirtbagLeague.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

const std::vector<LeagueRegular>& TheRegulars() {
  // **Worse than the comp field, and that is the point** -- the good ones
  // are at a comp on a Saturday. These are the people who are always here
  // on a Wednesday, and after eight weeks you know all six.
  static const std::vector<LeagueRegular> kRegulars = {
      {"Dez", 1.0, RouteType::Power, RouteType::Technical,
       "Route setter. Climbs his own problems better than anybody and "
       "cannot read a slab to save his life."},
      {"Priya", 0.5, RouteType::Crimp, RouteType::Dyno,
       "Physio, so she never gets hurt and never lets you forget it."},
      {"Wes", 0.0, RouteType::Technical, RouteType::Power,
       "Been coming twenty years. Has never once warmed up."},
      {"Nula", -0.5, RouteType::Dyno, RouteType::Endurance,
       "Eighteen, improving faster than anyone here, and about to stop "
       "turning up on Wednesdays."},
      {"Bram", -1.0, RouteType::Endurance, RouteType::Crimp,
       "Runs the bar afterwards. That is the actual reason he comes."},
      {"Tovah", -1.5, RouteType::Crack, RouteType::Dyno,
       "Trad climber, in for the winter, faintly embarrassed to be here."},
  };
  return kRegulars;
}

CompDials LeagueCompDials(const LeagueDials& d) {
  CompDials cd;
  cd.problems = d.nightProblems;
  cd.attempts = d.nightAttempts;
  cd.localFieldShift = d.regularShift;
  return cd;
}

bool LeagueTonight(const League& l, int day) {
  return l.nextNight > 0 && l.nextNight == day;
}

int DaysUntilLeague(const League& l, int day) {
  if (l.nextNight <= 0 || l.nextNight < day) return -1;
  return l.nextNight - day;
}

LeagueNight LeagueDay(League& l, const Rng& worldRng, int day,
                      const LeagueDials& d) {
  LeagueNight out;
  if (l.fieldPoints.size() != TheRegulars().size()) {
    l.fieldPoints.assign(TheRegulars().size(), 0.0);
  }

  if (l.nextNight <= 0) {
    // The first one, somewhere inside the week -- so a career does not
    // always start on league night and the gym's rhythm is not the
    // player's calendar.
    Rng rng = worldRng.Derive("league");
    l.nextNight = day + 1 + rng.IntRange(0, std::max(0, d.everyDays - 1));
    return out;
  }

  // **Rolled forward the day after**, so the night itself stays enterable
  // for the whole of its day -- the same rule the Games live under.
  while (day > l.nextNight) {
    const int wasTonight = l.nextNight;
    l.nextNight += d.everyDays;
    // A week gone by is a week of the block gone by whether or not you
    // came. A block you skipped is a block you came last in, which is
    // exactly what happens if you stop turning up to a real one.
    l.weeksDone++;
    for (std::size_t i = 0; i < l.fieldPoints.size(); i++) {
      // The regulars turn up. Their week's points are a placing among
      // themselves, which is deterministic on the block and the week --
      // no scorecard was watched, so there is nothing to read.
      (void)i;
    }
    // **Ordered, then paid off the same curve the player is paid off.**
    //
    // The first version gave each regular a flat 20-50 for turning up
    // while the player took `CircuitPoints` -- a hundred for a win -- and
    // the two numbers were not on the same scale at all. Measured: the
    // probe won **sixty-five blocks out of sixty-five.** A table where
    // everybody is scored differently is not a table.
    if (l.lastClimbedNight != wasTonight) {
      std::vector<std::pair<double, int>> order;
      for (std::size_t i = 0; i < l.fieldPoints.size(); i++) {
        Rng form = worldRng.Derive("league-week#" + std::to_string(l.block) +
                                   "#" + std::to_string(l.weeksDone) + "#" +
                                   std::to_string(i));
        order.push_back({TheRegulars()[i].gradeOffset +
                             form.NextDouble() * 2.0,
                         static_cast<int>(i)});
      }
      std::sort(order.begin(), order.end(),
                [](const std::pair<double, int>& a,
                   const std::pair<double, int>& b) {
                  return a.first > b.first;
                });
      // The field is one smaller on a week you did not come, because you
      // were not in it.
      const int field = static_cast<int>(order.size());
      for (std::size_t i = 0; i < order.size(); i++) {
        l.fieldPoints[order[i].second] +=
            CircuitPoints(static_cast<int>(i) + 1, field);
      }
    }

    if (l.weeksDone >= d.weeksPerBlock) {
      const std::vector<CircuitStanding> table = LeagueTable(l);
      for (std::size_t i = 0; i < table.size(); i++) {
        if (table[i].isYou) out.place = static_cast<int>(i) + 1;
      }
      out.blockClosed = true;
      // **The zero-tie rule is `LeagueTable`'s, not this line's.**
      //
      // This read `out.place == 1 && l.yourPoints > 0.0`, and the extra
      // clause was never the mechanism: deleting it failed no test and
      // changed no outcome, because a climber on nothing is already sorted
      // behind everybody else on nothing by the table itself. A guard that
      // cannot fire is a claim the code does not back, and this project
      // has removed one before for exactly this reason. The rule it was
      // guarding is tested where it lives.
      out.won = out.place == 1;
      if (out.won) {
        l.blockWins++;
        out.cash = d.blockCash;
        out.rep = d.blockRep;
        out.news = "You won the winter league. There is a laminated sheet "
                   "on the wall with your name on it.";
      } else if (l.yourPoints > 0.0) {
        out.news = "The league block is done. You finished " +
                   std::to_string(out.place) + " of " +
                   std::to_string(static_cast<int>(table.size())) + ".";
      }
      l.block++;
      l.weeksDone = 0;
      l.yourPoints = 0.0;
      l.fieldPoints.assign(TheRegulars().size(), 0.0);
    }
  }
  return out;
}

CompState SetTheLeagueBoard(const Rng& worldRng, double yourGrade, int day,
                            const LeagueDials& d) {
  // **Its own builder rather than `SetTheBoard`**, because a league board
  // is a different object: a wide spread priced by absolute grade, where a
  // comp board is a narrow spread priced by where each problem sits
  // relative to the rest. Sharing the comp's builder is what made the
  // personal best saturate in a month.
  static const char* kColours[7] = {"yellow", "green",  "blue", "red",
                                    "purple", "orange", "black"};
  CompState c;
  c.tier = CompTier::Local;
  c.attemptsLeft = d.nightAttempts;
  Rng rng = worldRng.Derive("league#" + std::to_string(day));

  const int n = std::max(1, d.nightProblems);
  for (int i = 0; i < n; i++) {
    const double t = n == 1 ? 0.0
                            : static_cast<double>(i) /
                                  static_cast<double>(n - 1);
    const double bump = d.spreadLow + (d.spreadHigh - d.spreadLow) * t;
    const int grade = std::max(
        0, std::min(kMaxGrade,
                    static_cast<int>(std::lround(yourGrade + bump))));
    CompProblem p;
    p.type = static_cast<RouteType>(i % 6);
    p.route = BuildRoute(rng, kColours[i % 7], grade, grade, p.type,
                         Discipline::Boulder);
    // The whole point: what it is worth is what it is, so the number grows
    // as the room does.
    p.topPoints = d.pointsPerGrade * static_cast<double>(grade + 1);
    p.flashPoints = p.topPoints * 1.3;
    p.zonePoints = p.topPoints * 0.4;
    c.problems.push_back(p);
    c.progress.push_back(ProblemProgress{});
  }
  return c;
}

LeagueResult SettleLeague(League& l, const CompState& comp, double yourGrade,
                          int day, const Rng& compRng,
                          const LeagueDials& d) {
  const CompDials cd = LeagueCompDials(d);
  LeagueResult out;
  out.score = YourScore(comp, cd);
  out.board.push_back(CompEntrant{"You", out.score, true, false});

  Rng rng = compRng.Derive("regulars");
  for (const LeagueRegular& r : TheRegulars()) {
    CompEntrant e;
    e.name = r.name;
    e.score = CompetitorScore(comp.problems,
                              yourGrade + r.gradeOffset + d.regularShift,
                              r.signature, r.weakness,
                              cd.formLow + rng.NextDouble() * cd.formRange,
                              cd);
    out.board.push_back(e);
  }
  std::stable_sort(out.board.begin(), out.board.end(),
                   [](const CompEntrant& a, const CompEntrant& b) {
                     if (a.score != b.score) return a.score > b.score;
                     // A tie at zero is a room full of people who did not
                     // climb, here as everywhere else.
                     if (a.score <= 0.0) return !a.isYou && b.isYou;
                     return a.isYou && !b.isYou;
                   });
  out.fieldSize = static_cast<int>(out.board.size());
  for (std::size_t i = 0; i < out.board.size(); i++) {
    if (out.board[i].isYou) out.place = static_cast<int>(i) + 1;
    if (!out.board[i].isYou) continue;
  }

  // Block points, off the card that was just climbed and **on the same
  // curve for everybody**. The regulars are paid here rather than by the
  // night tick, for the same reason the World Cup's table is fed from the
  // scorecard: the climber the night says won it has to be the climber the
  // block says won it.
  //
  // The top-weighted circuit curve, because a table is a table. **No
  // ranking points** -- that is the whole design.
  if (l.fieldPoints.size() != TheRegulars().size()) {
    l.fieldPoints.assign(TheRegulars().size(), 0.0);
  }
  for (std::size_t i = 0; i < out.board.size(); i++) {
    const int place = static_cast<int>(i) + 1;
    const CompEntrant& e = out.board[i];
    if (e.isYou) {
      l.yourPoints += CircuitPoints(place, out.fieldSize, cd);
      continue;
    }
    for (std::size_t r = 0; r < TheRegulars().size(); r++) {
      if (e.name == TheRegulars()[r].name) {
        l.fieldPoints[r] += CircuitPoints(place, out.fieldSize, cd);
        break;
      }
    }
  }
  l.nights++;
  l.lastClimbedNight = day;

  // **The number you are actually here for.**
  if (out.score > l.best) {
    out.improvedBy = out.score - l.best;
    out.personalBest = l.nights > 1;   // your first night is not a PB
    l.best = out.score;
    l.bestOnDay = day;
    if (out.personalBest) {
      out.rep = d.bestRep;
      out.news = "Best you have ever done on a league night, by " +
                 std::to_string(static_cast<int>(std::round(
                     out.improvedBy))) +
                 ".";
    }
  }
  if (out.place == 1 && out.score > 0.0) {
    out.cash = d.nightCash;
    if (out.news.empty()) out.news = "You won the night. Bram is buying.";
  }
  return out;
}

std::vector<CircuitStanding> LeagueTable(const League& l) {
  std::vector<CircuitStanding> table;
  table.push_back(CircuitStanding{"You", l.yourPoints, true, false});
  const std::vector<LeagueRegular>& regs = TheRegulars();
  for (std::size_t i = 0; i < regs.size() && i < l.fieldPoints.size(); i++) {
    table.push_back(
        CircuitStanding{regs[i].name, l.fieldPoints[i], false, false});
  }
  std::stable_sort(table.begin(), table.end(),
                   [](const CircuitStanding& a, const CircuitStanding& b) {
                     if (a.points != b.points) return a.points > b.points;
                     if (a.points <= 0.0) return !a.isYou && b.isYou;
                     return a.isYou && !b.isYou;
                   });
  return table;
}

std::string LeagueLine(const League& l, int day, const LeagueDials& d) {
  if (l.nextNight <= 0) return std::string();
  if (LeagueTonight(l, day)) {
    return "League night. $" +
           std::to_string(static_cast<int>(d.nightFee)) + ", ten goes, " +
           (l.best > 0.0 ? "and " + std::to_string(static_cast<int>(
                                        std::round(l.best))) +
                               " to beat."
                         : "and no score to beat yet.");
  }
  const int until = DaysUntilLeague(l, day);
  // **Silent unless it is close.** A whiteboard that counts down from six
  // days is a progress bar for a Wednesday.
  if (until < 0 || until > 2) return std::string();
  return until == 1 ? "League night tomorrow."
                    : "League night the day after next.";
}

}  // namespace dirtbag

#include "DirtbagComp.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

// The five types a comp sets. Crack is deliberately absent: nobody sets a
// crack on a competition wall, and a comp that could would be scoring the
// one discipline the gym cannot build.
const RouteType kCompTypes[5] = {RouteType::Crimp, RouteType::Power,
                                 RouteType::Endurance, RouteType::Technical,
                                 RouteType::Dyno};

// What a colour is called. Comp problems are named by tape, not by poetry.
const char* kColours[7] = {"Red",    "Blue",   "Yellow", "Green",
                           "Purple", "Orange", "Black"};

double TierOffset(CompTier t, const CompDials& d) {
  switch (t) {
    case CompTier::Local:    return d.localGradeOffset;
    case CompTier::Regional: return d.regionalGradeOffset;
    default:                 return d.nationalGradeOffset;
  }
}

double FieldShift(CompTier t, const CompDials& d) {
  switch (t) {
    case CompTier::Local:    return d.localFieldShift;
    case CompTier::Regional: return d.regionalFieldShift;
    default:                 return d.nationalFieldShift;
  }
}

}  // namespace

const char* TierName(CompTier t) {
  switch (t) {
    case CompTier::Local:    return "Local";
    case CompTier::Regional: return "Regional";
    default:                 return "National";
  }
}

int NextCompDay(int today, const CompDials& dials) {
  const int every = std::max(1, dials.everyDays);
  // The next multiple of the cadence at or after today. Day zero is not a
  // comp day -- a career does not open with one.
  const int n = (today + every - 1) / every;
  return std::max(every, n * every);
}

bool CompIsToday(int today, const CompDials& dials) {
  const int every = std::max(1, dials.everyDays);
  return today >= every && today % every == 0;
}

int DaysUntilComp(int today, const CompDials& dials) {
  if (CompIsToday(today, dials)) return 0;
  const int until = NextCompDay(today, dials) - today;
  return until <= dials.announceDaysAhead ? until : -1;
}

CompTier TierFor(double rankingPoints, const CompDials& dials) {
  if (rankingPoints >= dials.nationalAt) return CompTier::National;
  if (rankingPoints >= dials.regionalAt) return CompTier::Regional;
  return CompTier::Local;
}

const std::vector<Competitor>& TheField() {
  // **Offsets sit below you, not around you**, and the header says why: the
  // symmetric version measured 5th on average and 0% wins, forever.
  //
  // Each has one type they are known for and one they are soft on, which is
  // a redistribution rather than a buff -- the field's average strength is
  // unchanged and the *results* start to mean something.
  static const std::vector<Competitor> kField = {
      {"Kai",   +1.0, RouteType::Dyno,      RouteType::Crimp},
      {"Tess",   0.0, RouteType::Crimp,     RouteType::Dyno},
      {"Bowen", -1.0, RouteType::Power,     RouteType::Endurance},
      {"Marlo", -2.0, RouteType::Endurance, RouteType::Power},
      {"Quinn", -3.0, RouteType::Technical, RouteType::Power},
      {"Indra", -4.0, RouteType::Crimp,     RouteType::Technical},
      {"Remy",  -5.0, RouteType::Dyno,      RouteType::Endurance},
  };
  return kField;
}

CompState SetTheBoard(const Rng& worldRng, CompTier tier, double yourGrade,
                      int day, const CompDials& dials) {
  CompState c;
  c.tier = tier;
  c.attemptsLeft = dials.attempts;

  // Deterministic on (seed, day): the same comp is the same comp on a
  // reload, which is the no-reroll rule every gamble in this game lives
  // under. Reloading to get a friendlier board would make the whole thing
  // a save-scum exercise.
  Rng rng = worldRng.Derive("comp#" + std::to_string(day));

  const double base = yourGrade + TierOffset(tier, dials);
  for (int i = 0; i < dials.problems; i++) {
    const RouteType type = kCompTypes[i % 5];
    // A board is a spread, not five copies of one grade: two below the
    // tier's level, two at it, one above. That is what makes seven attempts
    // a decision -- the hard one is worth the most and might take three.
    const double bump = (i == 0 || i == 1) ? -1.0 : (i == 4 ? 1.0 : 0.0);
    const int grade =
        std::max(0, std::min(kMaxGrade,
                             static_cast<int>(std::lround(base + bump))));
    CompProblem p;
    p.type = type;
    p.route = BuildRoute(rng, kColours[i % 7], grade, grade, type,
                         Discipline::Boulder);
    // Worth what it asks. A problem a grade above the board pays about
    // half as much again as one a grade below it.
    p.topPoints = 10.0 + 3.0 * bump;
    p.flashPoints = p.topPoints * 1.3;
    p.zonePoints = p.topPoints * 0.4;
    c.problems.push_back(p);
    c.progress.push_back(ProblemProgress{});
  }
  return c;
}

AttemptResult AttemptProblem(CompState& comp, int problemIndex,
                             const Climber& climber, const Rng& compRng,
                             const CompDials& dials) {
  AttemptResult none;
  if (comp.finished) return none;
  if (comp.attemptsLeft <= 0) return none;
  if (problemIndex < 0 ||
      problemIndex >= static_cast<int>(comp.problems.size())) {
    return none;
  }
  ProblemProgress& pr = comp.progress[problemIndex];
  // A topped problem is done. Spending an attempt re-climbing it would be
  // the player throwing a go away, and a comp is short enough that the rule
  // is worth enforcing rather than trusting.
  if (pr.topped) return none;

  const CompProblem& p = comp.problems[problemIndex];

  AttemptInput in;
  in.climber = climber;
  in.route = p.route;
  // **Nerves, and this is the only dial that makes a comp different from a
  // session on the same holds.** Applied as a penalty on the odds rather
  // than a worse climber, because the climber is the same person -- it is
  // the situation that is harder.
  in.oddsPenalty = (1.0 - dials.pressure) * 2.0;
  in.padding = 1.0;      // it is a competition wall; the mats are the floor
  in.warmth = 1.0;       // you warmed up in isolation
  in.cleanliness = 1.0;  // freshly set

  // Its own stream per (problem, try), so a comp cannot shift the rng
  // anything else resolves on and a replay deals the same climb.
  Rng rng = compRng.Derive("go#" + std::to_string(problemIndex) + "#" +
                           std::to_string(pr.tries));
  const AttemptResult r = ResolveAttempt(rng, in);

  pr.tries++;
  comp.attemptsLeft--;
  if (r.sent) {
    pr.topped = true;
    pr.flashed = (pr.tries == 1);
    pr.zone = 2;
  } else if (!p.route.moves.empty()) {
    const double got = static_cast<double>(r.highpoint) /
                       static_cast<double>(p.route.moves.size());
    const int reached = got >= dials.highZoneAt   ? 2
                        : got >= dials.lowZoneAt  ? 1
                                                  : 0;
    pr.zone = std::max(pr.zone, reached);
  }
  if (comp.attemptsLeft <= 0) comp.finished = true;
  return r;
}

double YourScore(const CompState& comp, const CompDials& dials) {
  double total = 0.0;
  for (std::size_t i = 0; i < comp.problems.size(); i++) {
    const CompProblem& p = comp.problems[i];
    const ProblemProgress& pr = comp.progress[i];
    if (pr.flashed) {
      total += p.flashPoints;
    } else if (pr.topped) {
      total += p.topPoints;
    } else if (pr.zone >= 2) {
      total += p.zonePoints;
    } else if (pr.zone >= 1) {
      total += p.zonePoints * 0.5;
    }
  }
  (void)dials;
  return total;
}

double CompetitorScore(const std::vector<CompProblem>& problems, double grade,
                       RouteType signature, RouteType weakness, double form,
                       const CompDials& dials) {
  double total = 0.0;
  for (const CompProblem& p : problems) {
    // What grade this climber effectively operates at on *this* problem.
    const double eff = grade + (p.type == signature   ? dials.signatureShift
                                : p.type == weakness  ? -dials.signatureShift
                                                      : 0.0);
    const double asked = static_cast<double>(p.route.trueGrade);
    if (asked < eff - 0.5) {
      total += p.flashPoints;         // comfortably inside their level
    } else if (asked < eff + 0.5) {
      total += p.topPoints;           // at it
    } else if (asked < eff + 1.5) {
      total += p.topPoints * 0.5;     // a grade up: they might
    }
    // Above that, nothing. They are not getting up it either.
  }
  return total * form;
}

CompResult Settle(const CompState& comp, double yourGrade,
                  const std::string& rivalName, double rivalGrade,
                  const Rng& compRng, const CompDials& dials) {
  CompResult out;
  out.yourScore = YourScore(comp, dials);

  Rng rng = compRng.Derive("field");
  const auto form = [&]() {
    return dials.formLow + rng.NextDouble() * dials.formRange;
  };

  const double shift = FieldShift(comp.tier, dials);
  out.board.push_back(CompEntrant{"You", out.yourScore, true, false});
  for (const Competitor& c : TheField()) {
    CompEntrant e;
    e.name = c.name;
    e.score = CompetitorScore(comp.problems,
                              yourGrade + c.gradeOffset + shift, c.signature,
                              c.weakness, form(), dials);
    out.board.push_back(e);
  }
  double rivalScore = -1.0;
  if (!rivalName.empty()) {
    CompEntrant e;
    e.name = rivalName;
    e.isRival = true;
    // The rival rolls form like everybody else. Without it they post their
    // theoretical maximum every time while the field has off days, which is
    // exactly the inconsistency the 2D game shipped and then fixed.
    e.score = CompetitorScore(comp.problems, rivalGrade, RouteType::Power,
                              RouteType::Technical, form(), dials);
    rivalScore = e.score;
    out.board.push_back(e);
  }

  std::stable_sort(out.board.begin(), out.board.end(),
                   [](const CompEntrant& a, const CompEntrant& b) {
                     if (a.score != b.score) return a.score > b.score;
                     // **You take ties -- but only if you scored.** Without
                     // that clause, a climber who got nothing up at a comp
                     // two tiers above them was sorted ahead of everybody
                     // else who also got nothing up, came *fourth of eight*
                     // and collected top-half prize money for it. A tie at
                     // zero is not a tie, it is a room full of people who
                     // did not climb.
                     // Ordered *after* the other zeros rather than merely
                     // not before them: `stable_sort` keeps insertion order
                     // on a tie, and you are pushed onto the board first, so
                     // "return false" left you exactly where you started --
                     // ahead of them. It has to say so explicitly.
                     if (a.score <= 0.0) return !a.isYou && b.isYou;
                     return a.isYou && !b.isYou;
                   });

  out.fieldSize = static_cast<int>(out.board.size());
  for (std::size_t i = 0; i < out.board.size(); i++) {
    if (out.board[i].isYou) out.place = static_cast<int>(i) + 1;
  }
  out.beatTheRival = rivalScore >= 0.0 && out.yourScore > rivalScore;

  if (out.place == 1) {
    out.cash = dials.winCash;
    out.rep = dials.winRep;
  } else if (out.place <= 3) {
    out.cash = dials.podiumCash;
    out.rep = dials.podiumRep;
  } else if (out.place <= (out.fieldSize + 1) / 2) {
    out.cash = dials.topHalfCash;
    out.rep = dials.topHalfRep;
  } else {
    out.rep = dials.showedUpRep;
  }
  if (out.beatTheRival) out.rep += dials.beatRivalRep;
  return out;
}

std::string PlacingText(const CompResult& r) {
  if (r.place <= 0) return std::string();
  std::string s;
  switch (r.place) {
    case 1:  s = "You won it."; break;
    case 2:  s = "Second."; break;
    case 3:  s = "Third."; break;
    default:
      s = r.place <= (r.fieldSize + 1) / 2 ? "Mid-pack."
                                           : "Down the order.";
      break;
  }
  // The one line worth adding, because it is the thing you will remember
  // about a comp you came fifth in.
  if (r.beatTheRival) s += " You beat them, though.";
  return s;
}

}  // namespace dirtbag

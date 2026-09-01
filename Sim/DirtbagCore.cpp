#include "DirtbagCore.h"

#include <algorithm>

#include "DirtbagRng.h"

namespace dirtbag {

Climber NewClimber(const Rng& rng) {
  Rng r = rng.Derive("body-you-arrived-in");
  Climber c;
  // +/- 6 on a 7.14-point grade: under a grade of head start in any one
  // axis, enough that this career's easy style is not last career's.
  const auto skill = [&]() {
    return kStartingSkill + r.FloatRange(-6.0, 6.0);
  };
  c.skills.power = skill();
  c.skills.fingers = skill();
  c.skills.technique = skill();
  c.skills.endurance = skill();
  c.skills.head = skill();
  // Average stays the most common build, the way it is at any crag.
  const double m = r.NextDouble();
  c.morphology = m < 0.40   ? Morphology::Average
                 : m < 0.60 ? Morphology::Compact
                 : m < 0.80 ? Morphology::Lanky
                            : Morphology::Powerful;
  return c;
}

const char* RouteTypeName(RouteType type) {
  switch (type) {
    case RouteType::Crimp:     return "crimp";
    case RouteType::Power:     return "power";
    case RouteType::Endurance: return "endurance";
    case RouteType::Technical: return "technical";
    case RouteType::Dyno:      return "dyno";
    case RouteType::Crack:     return "crack";
  }
  return "crimp";
}

const char* CompDisciplineName(CompDiscipline d) {
  switch (d) {
    case CompDiscipline::Boulder: return "boulder";
    case CompDiscipline::Sport:   return "lead";
    case CompDiscipline::Speed:   return "speed";
  }
  return "boulder";
}

const char* BoulderGradeName(int grade) {
  static const char* kNames[] = {
      "V0", "V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9",
      "V10", "V11", "V12", "V13", "V14", "V15", "V16", "V17", "V18"};
  const int i = std::clamp(grade, 0, kMaxGrade);
  return kNames[i];
}

const char* SportGradeName(int grade) {
  // The 2D game's mapping: one V-index step ≈ one YDS step, topping out at
  // 5.16a ("Off the Charts" territory in both ladders).
  static const char* kNames[] = {
      "5.7",   "5.8",   "5.9",   "5.10a", "5.10c", "5.11a", "5.11c",
      "5.12a", "5.12c", "5.13a", "5.13c", "5.14a", "5.14b", "5.14c",
      "5.14d", "5.15a", "5.15b", "5.15c", "5.16a"};
  const int i = std::clamp(grade, 0, kMaxGrade);
  return kNames[i];
}

namespace {

HoldType SignatureHold(RouteType type) {
  switch (type) {
    case RouteType::Crimp:     return HoldType::Crimp;
    case RouteType::Power:     return HoldType::Sloper;
    case RouteType::Endurance: return HoldType::Pinch;
    case RouteType::Technical: return HoldType::Pocket;
    case RouteType::Dyno:      return HoldType::Dyno;
    case RouteType::Crack:     return HoldType::Crack;
  }
  return HoldType::Jug;
}

}  // namespace

Route BuildRoute(const Rng& worldRng, const std::string& name, int grade,
                 int trueGrade, RouteType type, Discipline discipline) {
  Route route;
  route.name = name;
  route.grade = grade;
  route.trueGrade = trueGrade;
  route.type = type;
  route.discipline = discipline;

  // Salting by name keeps every route's shape stable across the whole run —
  // the same line always has the same crux — without a stored move list.
  Rng rng = worldRng.Derive("route::" + name);

  // Boulders are short and dense; sport pitches are long with real rests.
  const int moveCount = discipline == Discipline::Boulder
                            ? rng.IntRange(5, 8)
                            : rng.IntRange(14, 20);
  const int cruxAt = rng.IntRange(moveCount / 3, moveCount - 2);

  for (int i = 0; i < moveCount; i++) {
    Move move;
    const double spread = rng.FloatRange(-1.2, 0.6);
    move.difficulty = static_cast<double>(trueGrade) + spread;
    move.crux = (i == cruxAt);
    if (move.crux) move.difficulty = trueGrade + rng.FloatRange(0.2, 0.9);

    // Most moves carry the route's signature hold; the rest vary.
    move.hold = rng.Chance(0.55) ? SignatureHold(type)
                                 : static_cast<HoldType>(rng.IntRange(0, 6));
    move.reachBias = rng.FloatRange(-1.0, 1.0);

    // Jugs are rests; sport lines also earn a mid-route shake-out stance.
    if (move.hold == HoldType::Jug && !move.crux) {
      move.restQuality = rng.FloatRange(0.3, 0.8);
    }
    // A pitch is not a long boulder. Real sport routes have stances every
    // few moves, and that is the whole reason a climber can stay on one for
    // twenty moves at their limit — measured without them, a V7.8 climber
    // sent a 19-move V7 pitch 0.9% of the time while sending the V7 boulder
    // 68%, because pump accrued for the full length with almost nowhere to
    // shake out. One guaranteed mid-route rest was not a pitch, it was a
    // boulder with a ledge in it.
    if (discipline != Discipline::Boulder && !move.crux && i % 4 == 3) {
      move.restQuality = std::max(move.restQuality, rng.FloatRange(0.35, 0.85));
    }
    if (discipline != Discipline::Boulder && i == moveCount / 2 && !move.crux) {
      move.restQuality = std::max(move.restQuality, rng.FloatRange(0.4, 0.9));
    }
    route.moves.push_back(move);
  }
  return route;
}

}  // namespace dirtbag

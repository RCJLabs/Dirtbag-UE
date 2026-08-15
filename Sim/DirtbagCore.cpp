#include "DirtbagCore.h"

#include <algorithm>

#include "DirtbagRng.h"

namespace dirtbag {

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
    if (discipline == Discipline::Sport && i == moveCount / 2 && !move.crux) {
      move.restQuality = std::max(move.restQuality, rng.FloatRange(0.4, 0.9));
    }
    route.moves.push_back(move);
  }
  return route;
}

}  // namespace dirtbag

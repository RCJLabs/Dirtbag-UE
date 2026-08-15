// Core types for the Dirtbag sim — plain structs, no engine types.
//
// These mirror the 2D game's data model (the reference implementation): the
// V0–V18 grade ladder with its YDS mapping, six route types, five skills,
// four morphologies. In Unreal these become USTRUCT mirrors fed by
// DataTables; here they stay POD so the standalone harness can test them.

#pragma once

#include <string>
#include <vector>

namespace dirtbag {

// --- Grades -----------------------------------------------------------------

// Grade is an index 0..18 (V0..V18). Sport routes map the same index onto
// YDS, as the 2D game does; both ladders are open-ended at the top.
constexpr int kMaxGrade = 18;

const char* BoulderGradeName(int grade);  // "V0".."V18"
const char* SportGradeName(int grade);    // "5.7".."5.16a"

// --- Routes -----------------------------------------------------------------

enum class RouteType { Crimp, Power, Endurance, Technical, Dyno, Crack };
enum class Discipline { Boulder, Sport };
enum class HoldType { Crimp, Sloper, Pinch, Pocket, Jug, Dyno, Crack };

struct Move {
  double difficulty = 0.0;   // absolute, on the grade scale (e.g. 7.4 ≈ soft V7 move)
  HoldType hold = HoldType::Jug;
  double restQuality = 0.0;  // 0 = no rest here, 1 = a full jug shake-out
  double reachBias = 0.0;    // -1 scrunchy .. +1 reachy; morphology reads this
  bool crux = false;
};

struct Route {
  std::string name;
  int grade = 0;             // the guidebook's opinion
  int trueGrade = 0;         // the rock's opinion; sandbagged when higher
  RouteType type = RouteType::Technical;
  Discipline discipline = Discipline::Boulder;
  std::vector<Move> moves;
};

// --- Climbers ---------------------------------------------------------------

struct Skills {
  // All 0..100, as in the 2D game.
  double power = 0.0;
  double fingers = 0.0;
  double technique = 0.0;
  double endurance = 0.0;
  double head = 0.0;
};

enum class Morphology { Compact, Average, Lanky, Powerful };

struct Climber {
  Skills skills;
  Morphology morphology = Morphology::Average;
  double skin = 9.0;    // session budget, 0..9; thin skin hurts crimps
  double psyche = 0.7;  // 0..1
};

// --- Conditions -------------------------------------------------------------

struct Conditions {
  // 0..1. ~0.5 is an ordinary day; the prime window pushes toward 1, summer
  // heat toward 0. The window is generated elsewhere — the session only
  // consumes the number.
  double friction = 0.5;
};

// --- Route generation -------------------------------------------------------

struct Rng;  // DirtbagRng.h

// Deterministically builds a route's move list from its identity: move
// difficulties spread around trueGrade with a crux spike, hold types drawn to
// match the route type, a rest or two on longer lines. Same route identity +
// seed → same moves, always.
Route BuildRoute(const Rng& worldRng, const std::string& name, int grade,
                 int trueGrade, RouteType type, Discipline discipline);

}  // namespace dirtbag

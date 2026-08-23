// Core types for the Dirtbag sim — plain structs, no engine types.
//
// These mirror the 2D game's data model (the reference implementation): the
// V0–V18 grade ladder with its YDS mapping, six route types, five skills,
// four morphologies. In Unreal these become USTRUCT mirrors fed by
// DataTables; here they stay POD so the standalone harness can test them.

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "DirtbagRng.h"

namespace dirtbag {

// Shared because more than one translation unit wants it, and because two
// file-local copies is not a saving: the Unreal build concatenates these
// units into one, where a second anonymous-namespace definition of the same
// signature is a redefinition error. The g++ harness compiles each file
// separately and never sees it.
inline double Clamp01(double v) { return std::max(0.0, std::min(1.0, v)); }

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
  //
  // These default to zero, which is right for a struct and wrong for a
  // person: a climber at 0 is not a beginner, they are somebody who cannot
  // pull on. Anything constructing a *climber* wants kStartingSkill.
  double power = 0.0;
  double fingers = 0.0;
  double technique = 0.0;
  double endurance = 0.0;
  double head = 0.0;
};

// What somebody who turns up at the Lot is made of. 50 is "solid
// intermediate" on the ladder in concepts/BALANCE-SKILL-LADDER.md — V5.0,
// which is where a new career starts and where an inherited one restarts.
//
// It is a named constant because it was previously *nowhere*: the engine's
// FDirtbagClimber defaulted every skill to 50 in its own header, the sim's
// Skills{} defaulted them to 0, and Inherit returned a plain PlayerState.
// So the second generation of every career was born with zero in all five,
// could not climb a V0, and could never be offered retirement — because
// that test needs a peak above zero and they never had one.
constexpr double kStartingSkill = 50.0;

enum class Morphology { Compact, Average, Lanky, Powerful };

// What goes wrong, and where it bites. Climbing injuries are specific —
// nobody has "an injury", they have a pulley or an elbow — and which one
// decides what you can still climb on, which is the whole of what makes an
// injury a decision rather than a pause.
enum class InjuryKind {
  Pulley,      // the classic: a finger, and crimps are over
  Lumbrical,   // pockets, and only pockets, and it takes forever
  Elbow,       // the slow one nobody rests properly; everything, a little
  Shoulder,    // slopers and anything dynamic; crimping is fine
};
constexpr int kInjuryKindCount = 4;

struct Injury {
  bool active = false;
  InjuryKind kind = InjuryKind::Pulley;
  double severity = 0.0;   // 0..1; what it costs and how long it holds you
  int daysLeft = 0;

  // **Who owns the clock.** `BodyDay` counts `daysLeft` down and clears
  // the injury when it hits zero; that was the whole of the injury model
  // through Phase 3. Phase 10's staged comeback is a second clock over the
  // same flag, and two owners of one flag disagree -- measured, before it
  // was fixed: a career took **316 cortisone shots and was hurt for 10,696
  // of 10,950 days**, because the comeback reached its last stage, the
  // injury was still flagged active, and the medical tick started the
  // whole thing again the next morning.
  //
  // Set by `StartComeback` and cleared when the injury heals. While it is
  // true `BodyDay` leaves the count alone and `DirtbagMedical` is the
  // authority -- it rewrites `daysLeft` nightly so everything that reads
  // it still reads the truth.
  bool staged = false;
};

struct Climber {
  Skills skills;
  Morphology morphology = Morphology::Average;
  double skin = 9.0;    // session budget, 0..9; thin skin hurts crimps
  double psyche = 0.7;  // 0..1

  // Training load, 0..100. The second body budget, and a much slower one
  // than skin: skin is back in six nights, load takes a month. It rises
  // with how hard you pull rather than how often, because that is what
  // hurts tendons — a mileage day on jugs costs skin and buys load nothing.
  //
  // This exists because two Phase 3 measurements in a row ended at the same
  // wall: skin was the only budget, so nothing money could buy added
  // climbing to a year, it only moved it around (notes/phase3-kit.md).
  double load = 0.0;

  // What is currently wrong with you.
  Injury injury;
};

// A climber as they arrive at the Lot: the starting body, and nothing else
// assumed. Use this rather than `Climber{}` anywhere a *person* is being
// made, because `Climber{}` is a zeroed struct and a person is not.
// A new climber, drawn from the world rather than stamped out. Skills sit
// around kStartingSkill with a spread of about three quarters of a grade,
// and morphology is rolled -- which matters, because the resolver reads it
// against every move's reachBias: a Lanky climber and a Compact one own
// different cruxes, which is the cheapest real variety a career can have.
//
// Deterministic from the rng, so the same world hands the same life the
// same body. There is deliberately no unseeded version: the one that
// stamped out flat 50s and Average, four times in a row, is what made four
// generations read word-for-word identical (ROADMAP 2026-08-22).
Climber NewClimber(const Rng& rng);

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

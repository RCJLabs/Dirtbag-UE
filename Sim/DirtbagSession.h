// The session resolver — the 2D game's send-odds model recast as a per-move
// simulation, so the 3D minigame layer has something real to drive.
//
// Contract with the presentation layer ("2D minigames, 3D staging"):
//   - The player's minigame performance enters as a per-move `execution`
//     scalar in [0,1] (HOLD TO CLIMB timing, load/latch windows). Headless,
//     a flat bot value stands in.
//   - The resolver returns a full per-move timeline (odds, pump, outcome) —
//     enough for the renderer to stage the whole attempt: where it got
//     desperate, where the shake-out saved it, where it ended.
//   - The sim, not the animation, decides everything. Same seed + inputs →
//     same attempt, always.

#pragma once

#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

enum class Style { Onsight, Flash, Redpoint, Sent, Fell };

// Every number a designer might turn, in one place, with its reason.
struct SessionDials {
  // Odds curve: how steeply per-move odds fall as move difficulty exceeds
  // effective skill. At 1.9, a fresh climber lands a move at their exact
  // level ~82% of the time (≈30% over a 6-move boulder — a project at your
  // limit), a move one grade over ~40%, two grades over ~9%.
  double oddsSlope = 1.9;

  // Pump: cost per move at difficulty parity, scaled up on harder moves and
  // down by endurance. Redline at 100 opens the hands — a forced fall.
  double basePumpCost = 11.0;
  double pumpPerDifficulty = 3.5;
  double enduranceRelief = 0.5;   // fraction of pump cost endurance can wipe at 100
  double restRecovery = 32.0;     // pump removed by a full-quality shake-out
  // Grades of ability lost at full pump. In grade units (not an odds
  // multiplier) so pump is felt exactly where the margin is thin: a crusher
  // on warmup jugs stays safe, the same climber at their limit does not.
  double pumpGradePenalty = 3.0;

  // Execution: how much the minigame matters. 0.5 means a perfect move beats
  // a botched one by ~25 points of effective skill — noticeable, not tyranny;
  // the stat block still decides who you are.
  double executionWeight = 0.5;

  // Conditions: full swing (0→1 friction) worth about half a letter grade.
  double frictionWeight = 6.0;

  // Skin: thin skin bites on crimps; falls cost the 2D game's 1 point.
  double thinSkinPenalty = 0.15;
  double fallSkinCost = 1.0;
  double sendSkinCost = 0.35;

  // Morphology: how loudly reach fit speaks on a biased move.
  double morphologyWeight = 4.0;
};

struct AttemptInput {
  Climber climber;
  Route route;
  Conditions conditions;
  double beta = 0.0;        // 0 = no knowledge, 1 = fully rehearsed
  int attemptNumber = 1;    // across the project's history, for style
  // Per-move minigame quality, 0..1. Missing entries fall back to botExecution.
  std::vector<double> execution;
  double botExecution = 0.72;
};

struct MoveResult {
  int index = 0;
  double odds = 0.0;      // the roll the move faced
  double pumpAfter = 0.0;
  bool success = false;
};

struct AttemptResult {
  bool sent = false;
  int highpoint = 0;              // moves completed
  Style style = Style::Fell;
  double skinCost = 0.0;
  double peakPump = 0.0;
  std::vector<MoveResult> timeline;
};

// Resolves one attempt. The rng should be a per-attempt derivation
// (e.g. sessionRng.Derive(routeName + "#3")) so attempts are independent
// and replayable.
AttemptResult ResolveAttempt(Rng& rng, const AttemptInput& input,
                             const SessionDials& dials = SessionDials{});

}  // namespace dirtbag

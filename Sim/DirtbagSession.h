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

  // Warmup: grades of ability missing when stone cold. The session loop
  // meters warmth (about two warmup boulders buy it all back); a bare
  // AttemptInput is fully warm by default so single-attempt callers and the
  // pre-loop tests are untouched.
  double coldStartPenalty = 1.5;

  // Psyche as ability, centered on 0.7 (an ordinary day). Being wrecked
  // (0.0) costs about a grade; being lit up (1.0) buys back less than half
  // of one — despair is louder than stoke, as anyone who has belayed a
  // heartbroken projecter knows.
  double psycheWeight = 1.5;

  // Shake-outs, live form: the first shake at a stance is its full value
  // (what the batch bot takes automatically); milking it further halves each
  // time and pays a flat hang tax, so a poor stance goes net-negative fast.
  // That falloff is the release-to-shake decision.
  double shakeDiminish = 0.5;
  double shakeHangCost = 4.0;

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
  double warmth = 1.0;      // 0 cold .. 1 warm; the session loop starts cold
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

// --- Live attempts -----------------------------------------------------------
//
// The batch resolver unrolled so the minigame can drive it move by move —
// SETUP.md step 4, the difference between watching a replay and driving an
// attempt. HOLD TO CLIMB fills each StepMove's execution as it happens;
// releasing is ShakeOut. ResolveAttempt is implemented on this core with a
// bot policy (one shake at every stance that offers one), so the batch and
// live forms cannot drift apart.

struct LiveAttempt {
  // Treat as opaque outside the sim; Begin/Peek/Step/Shake/Finish drive it.
  // Fields stay public so the harness can stage exact situations.
  AttemptInput input;   // its execution vector is unused; exec arrives per step
  SessionDials dials;
  Rng rng;
  double pump = 0.0;
  int nextMove = 0;
  int shakesAtStance = 0;
  bool over = false;
  AttemptResult partial;  // timeline/highpoint/peak so far; Finish completes it
};

LiveAttempt BeginAttempt(const Rng& rng, const AttemptInput& input,
                         const SessionDials& dials = SessionDials{});

// Odds the next move would face at this execution — the UI's "how close is
// this" readout. Pure: no rolls, no state change.
double PeekOdds(const LiveAttempt& la, double execution);

// Commit to the next move with the minigame's execution quality. After it
// the attempt may be over (fell, or topped out). Calling on a finished
// attempt is a no-op returning an empty MoveResult.
MoveResult StepMove(LiveAttempt& la, double execution);

// Release to shake out at the current stance. Returns net pump recovered
// (negative when the hang tax beat the stance). No-op before the first move
// or after the attempt ends.
double ShakeOut(LiveAttempt& la);

bool AttemptOver(const LiveAttempt& la);

// Styles, skin and the sent flag — same accounting as the batch resolver.
AttemptResult FinishAttempt(const LiveAttempt& la);

}  // namespace dirtbag

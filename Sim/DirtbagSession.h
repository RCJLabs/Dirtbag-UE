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
  // What a skill number *means* in grades — the ladder's whole calibration.
  // At span 14 / floor -2: skill 50 is a V5 climber, 65 is V7, 80 is V9,
  // 100 is V12, leaving the top of the ladder for the exceptional (both
  // ladders are open-ended at the top). The 0..100 range therefore spans a
  // whole climbing life, not a warmup: most careers live between 30 and 70.
  double skillGradeSpan = 14.0;
  double skillGradeFloor = -2.0;

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

  // Dirt, on a line nobody has cleaned. Deliberately brutal next to
  // friction: filthy rock is not "worse conditions", it is a different and
  // mostly impossible route, and the brush is the only answer. At 4 grades,
  // a virgin line sits far enough out of reach that cleaning is the first
  // move rather than an optimisation.
  double dirtGradePenalty = 4.0;

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

  // Skin, as a quality across its whole range rather than a cliff.
  //
  // This used to bite only below 3 and only on crimps, capping at 0.45
  // grades — which made skin 9 and skin 3 identical to climb on, and meant
  // resting was worth nothing at all: a season played fresh and a season
  // played wrecked came out the same, measured, because skin is conserved
  // and spreading it changed no outcome (notes/phase2-season-probe.md).
  //
  // Squared so the top of the range is nearly free and the bottom bites
  // hard, which is how skin actually goes: 9 to 7 is nothing, 3 to 1 is the
  // difference between climbing and not. Sized against the other ability
  // terms — under coldStartPenalty (1.5) and well under pumpGradePenalty
  // (3.0), because skin should shape a session rather than decide it.
  double skinGradePenalty = 1.4;   // grades lost on crimps at zero skin
  double freshSkin = 9.0;          // where the penalty reaches zero
  // Good holds still hurt on shot skin, just less. Sloper and jug days are
  // what you climb when your tips are gone, and that should be a real
  // option rather than a free one.
  double skinBiteOnGoodHolds = 0.35;

  // Dead rubber, priced like thin skin and for the same reason: it should
  // shape what you get on rather than decide it. Mirrors GearDials so the
  // resolver and the shop agree; the numbers live there.
  double deadShoeGradePenalty = 1.1;
  double shoeBiteOnGoodHolds = 0.3;

  double fallSkinCost = 1.0;
  double sendSkinCost = 0.35;

  // Morphology: how loudly reach fit speaks on a biased move.
  double morphologyWeight = 4.0;
};

// A skill number (0..100) as a grade on the V ladder. The single source of
// this mapping: the resolver reads it to price a move, the day loop reads it
// to decide whether an attempt was hard enough to train anything. Two copies
// of this formula would drift, and the drift would be invisible.
double SkillToGrade(double skill, const SessionDials& dials = SessionDials{});

// What this climber can do on this route's kind of holds, in grades —
// the ground-up read, before pump, execution, or luck get a say.
double AbilityOnRoute(const Climber& climber, const Route& route,
                      const SessionDials& dials = SessionDials{});

// Reading the line from the ground. Judges against the route's *guidebook*
// grade, never its true grade: a sandbag is supposed to look reasonable
// right up until you're on it.
enum class RouteRead { Warmup, Comfortable, AtYourLimit, Project, NotThisYear };

RouteRead ReadRoute(const Climber& climber, const Route& route,
                    const SessionDials& dials = SessionDials{});

// The read in the game's own voice. Text lives here for now; it moves to
// DataTables when route descriptions become content.
const char* ReadRouteText(RouteRead read);

struct AttemptInput {
  Climber climber;
  Route route;
  Conditions conditions;
  double beta = 0.0;        // 0 = no knowledge, 1 = fully rehearsed
  int attemptNumber = 1;    // across the project's history, for style
  double warmth = 1.0;      // 0 cold .. 1 warm; the session loop starts cold
  double cleanliness = 1.0; // 1 clean rock .. 0 never been touched
  double shoeWear = 0.0;    // 0 new rubber .. 1 dead; see DirtbagGear.h
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

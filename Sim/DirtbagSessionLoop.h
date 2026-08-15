// The layer above single attempts: a day's session (warmup → attempts →
// skin budget) and a project's memory across attempts and sessions.
//
// This is the 2D game's projecting loop made explicit. The resolver
// (DirtbagSession.h) answers "how does this one burn go?"; this layer answers
// the questions a session actually asks — am I warm yet, how much skin is
// left, do I know the moves, is this attempt #1 or #14 — and feeds the
// answers into the resolver as state. Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"
#include "DirtbagSession.h"

namespace dirtbag {

// Every number a designer might turn, with its reason.
struct SessionLoopDials {
  // Warmth gained per move climbed: ~12 easy moves (two warmup boulders)
  // reach full warmth, matching the 2D game's warmup rhythm. Skipping the
  // warmup and pulling straight onto the project costs coldStartPenalty
  // grades (SessionDials) — a choice, not a bug.
  double warmupPerMove = 0.085;

  // Beta: falling on a move still teaches it, so an attempt converts half
  // the gap between what you knew and what you touched. Repeating known
  // ground refines slowly, and never past the ground actually touched —
  // lapping the start does not unlock the crux.
  double betaLearnRate = 0.5;
  double betaRehearsalGain = 0.04;

  // Psyche swings, kept small: a session shapes mood, it doesn't rewrite it.
  // Sends feed it, a new highpoint feeds it less, a burn that goes nowhere
  // drains it. The floor exists because no session ends at literal zero
  // will to live.
  double psycheSendGain = 0.15;
  double psycheHighpointGain = 0.05;
  double psycheFailLoss = 0.05;
  double psycheFloor = 0.05;
};

// One route's history across attempts and sessions — the projecting ledger.
// Persists between sessions; will live in the save file from Phase 1.
struct ProjectMemory {
  std::string routeName;
  int attempts = 0;          // lifetime burns; feeds AttemptInput::attemptNumber
  int bestHighpoint = 0;     // moves completed, best ever
  double beta = 0.0;         // 0..1 move knowledge; feeds AttemptInput::beta
  bool sent = false;
  Style firstSendStyle = Style::Fell;  // how it first went down, forever
};

// A day's climbing body-state, from the session's first pull-on to its last.
struct SessionState {
  double skinLeft = 9.0;  // starts at the climber's skin; every burn spends it
  double warmth = 0.0;    // 0 cold .. 1 warmed up
  double psyche = 0.7;    // starts at the climber's; swings with the session
  int attemptsMade = 0;   // across all routes this session
};

SessionState StartSession(const Climber& climber);

// One burn. Derives the attempt rng from the session stream (per route, per
// lifetime attempt number — replayable, never shared), applies session state
// and project memory to the resolver, then pays the session and updates the
// ledger. The returned timeline is the presentation layer's script, exactly
// as with a bare ResolveAttempt.
AttemptResult AttemptInSession(const Rng& sessionRng, SessionState& session,
                               ProjectMemory& memory, const Climber& climber,
                               const Route& route, const Conditions& conditions,
                               const std::vector<double>& execution = {},
                               double botExecution = 0.72,
                               const SessionDials& dials = SessionDials{},
                               const SessionLoopDials& loop = SessionLoopDials{});

}  // namespace dirtbag

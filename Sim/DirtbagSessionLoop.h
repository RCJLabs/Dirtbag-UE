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

#include "DirtbagCharacter.h"
#include "DirtbagCore.h"
#include "DirtbagHabits.h"
#include "DirtbagAilments.h"
#include "DirtbagBodyContext.h"
#include "DirtbagMedical.h"
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

  // Below this you are cold enough that it is costing you real grades, and
  // the fix is two easy problems rather than another go at the project.
  double coldBelowWarmth = 0.45;

  // Skin left at which the session is telling you something. The first is
  // "pick something with better holds", the second is "that is the day".
  double thinSkin = 3.0;
  double spentSkin = 1.2;
};

// One route's history across attempts and sessions — the projecting ledger.
// Persists between sessions; will live in the save file from Phase 1.
struct ProjectMemory {
  std::string routeName;
  int grade = -1;            // the guidebook grade, recorded on first touch;
                             // -1 = unknown (a pre-v2 save, see DirtbagSave)
  int attempts = 0;          // lifetime burns; feeds AttemptInput::attemptNumber
  int bestHighpoint = 0;     // moves completed, best ever
  double beta = 0.0;         // 0..1 move knowledge; feeds AttemptInput::beta
  bool sent = false;
  Style firstSendStyle = Style::Fell;  // how it first went down, forever

  // --- First ascents ---------------------------------------------------
  // An unclimbed line is dirty, and dirt is the first thing standing between
  // you and it. 1.0 is clean rock; a virgin line starts filthy and every
  // hour on the brush buys some back. This lives in the ledger rather than
  // on the crag because it is the one fact about a line that the player
  // changes and the save file therefore has to carry.
  double cleanliness = 1.0;

  // The name you gave it, once it went and the naming was yours to do.
  // Never replaces routeName: routeName is this ledger's key, and moving it
  // would orphan every burn already recorded here.
  std::string givenName;

  // True when this player did the line first. The claim the career is built
  // on, so it is recorded rather than inferred.
  bool firstAscent = false;

  // What it actually turned out to be, once somebody had done it. -1 until
  // then, because before the first ascent every grade at a project is an
  // opinion.
  int confirmedGrade = -1;

  // Boulder or pitch. The ledger records what you did, and which of the two
  // it was is part of that — a guidebook entry for a rope route reads
  // "5.12a" and one for a boulder reads "V7", and a career card that cannot
  // tell them apart will confidently print the wrong ladder.
  Discipline discipline = Discipline::Boulder;
};

// A day's climbing body-state, from the session's first pull-on to its last.
struct SessionState {
  double skinLeft = 9.0;  // starts at the climber's skin; every burn spends it
  double warmth = 0.0;    // 0 cold .. 1 warmed up
  double psyche = 0.7;    // starts at the climber's; swings with the session
  int attemptsMade = 0;   // across all routes this session
  double shoeWear = 0.0;  // the pair you pulled on with today
  // How covered the landing is, 0 bare ground .. 1 as padded as it gets.
  // Carried on the session rather than read from the kit each burn, because
  // what matters is what you dragged in, not what you own back at the van.
  // Defaults to fully padded so that every caller predating pads — the
  // golden vectors included — resolves exactly as it always did.
  double padding = 1.0;

  // **And what is on your harness.** Carried on the session for exactly the
  // reason the padding is: what matters is what you walked in with, not
  // what is sitting in the van an hour down the hill. Empty by default, so
  // every caller predating trad — the golden vectors included — resolves
  // exactly as it always did, and a leader who forgot the rack solos the
  // pitch, which is the honest answer rather than a crash.
  Rack rack;

  // **What the person climbing costs and learns**, copied off the career
  // when the session starts, exactly like the padding and the rack.
  //
  // On the session rather than passed down through CommitAttempt because
  // that is the one place every path already meets: the batch loop, the
  // live attempt the minigame drives, and the engine's own commit all read
  // a SessionState, and a parameter would have reached one of the three.
  // See Sim/DirtbagHabits.h.
  double betaRate = 1.0;   // how fast this climber wires a line
  double skinRate = 1.0;   // what a burn costs their tips
};

SessionState StartSession(const Climber& climber);

// What the session would tell you if it could — the thing a player standing
// at the bottom of a line cannot see and a climber standing there would
// know instantly.
//
// This exists because of a measured trap: warmth is earned per move
// climbed, so a line you cannot start is a line you can never warm up on,
// and being cold makes it harder to start. A season probe spent 519 burns
// getting one move up a line, averaging warmth 0.33, with nothing anywhere
// saying "warm up first" or "this is not happening today"
// (notes/phase2-season-probe.md).
enum class SessionAdvice {
  Ready,          // warm, skinned, get on it
  Cold,           // warm up on something easy first
  SkinThin,       // your tips are gone; jugs or go home
  Wrecked,        // both, and the day is over
};

SessionAdvice ReadSession(const SessionState& session, const Climber& climber,
                          const SessionDials& dials = SessionDials{},
                          const SessionLoopDials& loop = SessionLoopDials{});

// The advice in the game's voice.
const char* SessionAdviceText(SessionAdvice advice);

// The three pieces of a session burn, exposed separately so a live
// (player-driven) attempt can use them around BeginAttempt/StepMove:
// derive the burn's rng, assemble the resolver input from session state and
// project memory, and afterwards pay the session and update the ledger.
// AttemptInSession is exactly these three around a batch ResolveAttempt.
Rng DeriveAttemptRng(const Rng& sessionRng, const ProjectMemory& memory,
                     const Route& route);
// `who` is the person rather than the body. The only thing read off it here
// is the flaw's odds penalty, because a flaw is the one part of identity
// allowed anywhere near send odds -- see Sim/DirtbagCharacter.h, where
// origins are deliberately kept out of the resolver so that where you came
// from can never be a difficulty setting. Defaulted to nobody, and nobody is
// neutral, so every existing caller and every golden vector resolves exactly
// as it did.
AttemptInput BuildSessionAttemptInput(
    const SessionState& session, const ProjectMemory& memory,
    const Climber& climber, const Route& route, const Conditions& conditions,
    const std::vector<double>& execution = {}, double botExecution = 0.72,
    const Character& who = Character{},
    // What the joints carry, which does not heal. Defaulted to a clean
    // file, and a clean file is neutral, so every existing caller and
    // every golden vector resolves exactly as it did.
    const Medical& med = Medical{}, int day = 0,
    // Being ill, and the tooth. Both neutral by default.
    const Sickness& sick = Sickness{}, const Teeth& teeth = Teeth{},
    // ...and how you have been climbing, which reaches an attempt the same
    // way a temperament does. Neutral by default like the rest of the tail.
    const Quirks& quirks = Quirks{}, const Logbook& logbook = Logbook{},
    // ...and the life outside climbing, of which exactly one thing reaches
    // an attempt: how steady a phone call home leaves you. Neutral by
    // default like the rest of the tail -- an empty life is a life with
    // nothing in it, which is where every career starts.
    const Life& life = Life{});
void CommitAttempt(SessionState& session, ProjectMemory& memory,
                   const Route& route, const AttemptResult& result,
                   const SessionLoopDials& loop = SessionLoopDials{});

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
                               const SessionLoopDials& loop = SessionLoopDials{},
                               // Who you are, and what the joints carry.
                               // Both neutral by default, so every existing
                               // caller resolves exactly as it did.
                               const Character& who = Character{},
                               const Medical& med = Medical{}, int day = 0,
                               const Sickness& sick = Sickness{},
                               const Teeth& teeth = Teeth{},
                               const Quirks& quirks = Quirks{},
                               const Logbook& logbook = Logbook{},
                               const Life& life = Life{});

}  // namespace dirtbag

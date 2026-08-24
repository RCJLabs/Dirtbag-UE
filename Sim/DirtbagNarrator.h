// The narrator — what somebody watching would say.
//
// `DIRTBAG.md` §9 names the project's top risk as *"session isn't tense or
// legible in 3D"*, and Phase 5 calls staging the craft centre. Most of that
// needs an Editor. **This part does not.**
//
// The resolver has always returned a full per-move timeline — the odds each
// move faced, the pump after it, whether it stuck, and now what was on the
// rope while it happened. Nothing anywhere turned that into words. There
// were fragments (`HowCloseText`, `RunoutText`, `PieceText`, `BelayText`)
// and no layer that read a whole attempt and said what happened, so a
// twenty-move pitch and a six-move boulder produced the same silence.
//
// **The narrator's job is knowing when to shut up.**
//
// A line per move is not commentary, it is a log. Twenty lines for a pitch
// is twenty lines nobody reads, and the moment that mattered is somewhere in
// the middle of them. So this file emits a beat only when something
// *happened* — the crux arriving, a move that should not have stuck, a
// shake-out that bought the route back, the forearms crossing over, getting
// off the deck, a piece that went in badly, the hands opening. Everything
// else is quiet, and the quiet is what makes the beats land.
//
// Two rules it does not break:
//
//   **It reads; it never recomputes.** Every beat is derived from the
//   result the sim already produced. If the narrator wants to know
//   something the result does not carry, that is a signal the *result*
//   should carry it — which is exactly how `AttemptResult::gear` came to
//   exist for trad. A narrator that recomputed odds would be a second
//   opinion about what happened, and the first one it disagreed with would
//   be a bug nobody could find.
//
//   **Never a number.** The gate this project keeps setting for its own
//   text is that somebody watching over your shoulder knows what happened
//   without reading a stat line -- `HowCloseText` says so in as many words.
//   "You fell at move 9 of 12" is a thing you read; "one move, that was the
//   go" is a thing you see.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagSession.h"

namespace dirtbag {

// What kind of moment this is. The camera and the sound key off this rather
// than off the words, so that restaging a beat does not mean re-reading it.
enum class BeatKind {
  Ground = 0,    // before you pull on: what you are getting on, and today
  OffTheDeck,    // the first piece, and the ground stops being the question
  Placed,        // gear went in, and what it is worth
  Crux,          // the hard one, arriving
  NearlyBlew,    // a move that had no business staying on
  Shake,         // a rest that actually worked
  Pumped,        // the forearms crossing over
  Runout,        // a long way above whatever is holding the rope
  Fell,
  Topped,
  kBeatKindCount
};
constexpr int kBeatKindCount = static_cast<int>(BeatKind::kBeatKindCount);

const char* BeatKindName(BeatKind kind);

struct Beat {
  // Which move this happened on, or -1 for the ground. The staging layer
  // scrubs the timeline by this.
  int move = -1;
  BeatKind kind = BeatKind::Ground;

  // **How loud it is**, 0 background .. 1 the moment of the attempt.
  //
  // Derived rather than authored, and it exists because the presentation
  // layer has three lines of room and an attempt may have eight beats. A
  // HUD takes the loudest; a camera pushes in past a threshold; a replay
  // shows the lot. Without it every caller invents its own ranking and
  // they disagree.
  double weight = 0.0;

  std::string line;
};

struct NarratorDials {
  // --- When to speak -----------------------------------------------------
  //
  // Every one of these is a threshold on something the resolver already
  // measured. They are set high on purpose: the failure mode of a narrator
  // is not missing a moment, it is narrating a Tuesday.

  // A move that stuck against odds this bad had no business staying on.
  // Under the resolver's own curve a fresh climber lands a move at their
  // exact level about 82% of the time, so a third is genuinely desperate.
  double nearlyBlewBelow = 0.34;

  // ...and a crux that was never in doubt is not a crux worth mentioning.
  // The crux beat is suppressed above this, because a route being harder
  // in the middle is a fact about the route and not a moment.
  double cruxIsSafeAbove = 0.86;

  // Where the forearms cross over, and where they are gone. Two lines and
  // never more: the pump bar is on screen and saying it every move is the
  // bar's job, not the narrator's.
  double forearmsGoingAt = 62.0;
  double nearlyGoneAt = 88.0;

  // A shake-out worth mentioning is one that changed the attempt. Under
  // this it is a chalk-up.
  double shakeWorthIt = 12.0;

  // Above the gear. In runout units, where 1 is as far above it as fear
  // goes -- see DirtbagSport.h.
  double runoutWorthSayingAt = 0.55;

  // When a line has stopped being a route and started being a project.
  // Both are about what the *ground* beat says, because after the first go
  // the interesting thing is not what the route looks like -- it is how
  // many times you have been here.
  double knowsItAt = 0.85;      // beta at which you know every move
  int lostCountAt = 12;         // goes, before nobody is counting any more

  // A piece worth remarking on is one you do not trust. A bomber cam every
  // four moves is not news, and saying so five times a pitch is how a
  // narrator teaches you to stop reading it.
  double pieceWorthSayingBelow = 0.55;

  // --- How loud ----------------------------------------------------------
  //
  // Beats are weighted so a caller with three lines takes the right three.
  // An ending always outranks a middle: the last thing that happened is the
  // thing somebody watching would tell you about.
  double endingWeight = 1.0;
  double groundWeight = 0.30;
  double offTheDeckWeight = 0.45;
  double placedWeight = 0.42;
  double cruxWeight = 0.70;
  double nearlyBlewWeight = 0.66;
  double shakeWeight = 0.50;
  double forearmsWeight = 0.55;
  double nearlyGoneWeight = 0.76;
  double runoutWeight = 0.62;
  // How much of a beat's loudness comes from *when* it happened. A crux at
  // move three and the same crux at move nineteen are not the same moment,
  // because by move nineteen you have spent an afternoon getting there.
  double lateness = 0.25;
};

// **The whole attempt, as a watcher would tell it.**
//
// Works on a finished attempt and on one still going: `LiveAttempt::partial`
// is an `AttemptResult`, so the staging layer calls this after every move
// and takes what is new. Deliberately one function rather than a batch form
// and a live form -- this project has twice shipped two paths that drifted,
// and the narrator is the last place that should be allowed to say two
// different things about the same climb.
//
// Whether the attempt is over is *derived* rather than passed: the last
// timeline entry failing is a fall and a full highpoint is a top-out, and
// anything else is a climber who is still on the wall.
std::vector<Beat> CallTheAttempt(const AttemptInput& input,
                                 const AttemptResult& result,
                                 const NarratorDials& dials = NarratorDials{});

// The loudest few, in the order they happened -- for a caller with three
// lines of room rather than eight. Ranked by weight, then re-sorted by
// move, because a highlight reel out of order is not a highlight reel.
//
// `n` at or below zero returns nothing, which is a caller saying it has no
// room rather than a caller asking for everything.
std::vector<Beat> Loudest(const std::vector<Beat>& beats, int n);

// The newest thing worth saying, for a live attempt driving the wall move
// by move. Empty line and `move == -1` when the right answer is silence,
// which it usually is.
Beat LastWord(const AttemptInput& input, const AttemptResult& result,
              const NarratorDials& dials = NarratorDials{});

// One sentence for the day's log -- what happened, and why, in the order a
// climber would say it. This is the line that goes in a career's history,
// so it names the cause rather than the position: *off at the crux, and the
// pump was already gone* rather than *fell on move nine*.
std::string HowItWent(const AttemptInput& input, const AttemptResult& result,
                      const NarratorDials& dials = NarratorDials{});

}  // namespace dirtbag

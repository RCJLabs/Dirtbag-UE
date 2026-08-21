// Ethics: the things you did, and the day somebody finds out.
//
// "Ethics are gameplay. Style, chipping, bolting, closures, sponsorship
// compromise — multi-beat arcs whose consequences resurface seasons later"
// (concepts/DIRTBAG.md). That last clause is the whole design. Factions
// already price the things you do openly, instantly, the day you do them.
// This is the other kind of act: the one nobody sees.
//
// So an ethical shortcut is not a transaction, it is a **secret with a
// fuse**. You take the benefit now — the line goes, the grade is yours, the
// sponsor is happy — and you carry a thing that can come out. It usually
// comes out years later, and the mechanism is the cruel part:
//
//     **The more people are watching you, the more likely it is that
//     somebody noticed.**
//
// Discovery scales with visibility, and visibility is Scene standing plus
// whatever your sponsor has made of you. A nobody who chips a hold gets
// away with it for a very long time. A star who chipped a hold at
// twenty-five is found out at thirty-two, when it costs everything. Success
// is what exposes you, which is both true and the only version of this
// worth playing.
//
// And when it comes out, the ascent goes with it. Your hardest send
// vanishing from the record is what gives this teeth — it cascades straight
// into a sponsor's next review.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagFactions.h"
#include "DirtbagRng.h"
#include "DirtbagSponsor.h"

namespace dirtbag {

// The five shortcuts. Each buys something real and each is a thing you
// would not say out loud at the fire.
enum class EthicalAct {
  ChippedAHold,   // it goes now. It did not before, and it never will again.
  RetroBolted,    // somebody's ground-up line, made safe without asking
  ClaimedASend,   // the oldest one in the sport
  StagedAPhoto,   // the sponsor wanted a shot of a send that did not happen
  PulledOnGear,   // one hang nobody saw, and you called it clean
};
constexpr int kEthicalActCount = 5;

const char* EthicalActName(EthicalAct act);

// Does the truth take the ascent with it? Chipping and retro-bolting change
// the rock and the send stands, hollow but real. Claiming, staging and
// pulling on gear are lies about what happened, and those ascents go.
bool StripsTheAscent(EthicalAct act);

struct Secret {
  EthicalAct act = EthicalAct::ChippedAHold;
  std::string routeKey;    // the line it was done to; empty for a staged shot
  int dayDone = 0;
  bool known = false;
  int dayFound = 0;
};

struct EthicsDials {
  // Chance per day that an unknown secret surfaces, before visibility.
  //
  // Sized against the two careers this has to produce, not guessed. At
  // 0.000063 a complete unknown has rather better than even odds of it
  // **never coming out at all** across thirty years — which is the career
  // most people who do this actually have, and the thing that makes the
  // choice tempting rather than a trap with a timer on it.
  //
  // The first pass had this at 0.00035 with a 14x multiplier, which caught
  // 98% of nobodies and found a star out in six months. A fuse that short
  // is not an arc: the point is that the person who is caught is not quite
  // the person who did it.
  double baseDiscoveryPerDay = 0.000063;

  // What being watched multiplies it by, at full visibility. At 5, a star
  // is found out around five years in — long enough that the act was made
  // by somebody younger, and soon enough that it lands on the career they
  // built instead.
  double visibilityMultiplier = 5.0;

  // A fresh secret is safest: the people who were there have not compared
  // notes yet, and nobody is looking. This is how long that lasts.
  int quietDays = 45;

  // What it costs when it comes out, per act, as a Standing hit with the
  // old guard. Chipping is the unforgivable one; everything else is a
  // matter of degree.
  double costChipped = 0.85;
  double costRetroBolted = 0.6;
  double costClaimed = 0.7;
  double costStaged = 0.45;
  double costPulledOn = 0.3;

  // The Scene is not innocent, but it is not the injured party either: it
  // punishes being *caught* rather than the act, and less hard.
  double sceneShareOfTheCost = 0.5;

  // Psyche. Being found out is not only arithmetic.
  double psycheCost = 0.25;
};

// Doing it. The caller applies whatever the act buys — this records only
// that it happened, because the benefit differs per act and belongs where
// it is felt.
Secret Commit(EthicalAct act, const std::string& routeKey, int day);

// 0..1: how closely you are watched. Scene standing plus what a sponsor has
// made of you — success is what exposes you.
double VisibilityFrom(const Standing& standing, SponsorTier tier);

// Does anything come out today? Returns the index of the secret that
// surfaced, or -1. Only one at a time: a career unravelling in a single
// afternoon is a punishment, and this is meant to be a story.
int SomebodyFindsOut(std::vector<Secret>& secrets, const Rng& worldRng,
                     int day, double visibility,
                     const EthicsDials& dials = EthicsDials{});

// What it costs. Call once, when it comes out.
void ItComesOut(const Secret& secret, Standing& standing, double& psyche,
                const EthicsDials& dials = EthicsDials{});

// "Everyone knows about the holds on Chalk Ghost now. It was nine years
// ago. It does not matter that it was nine years ago."
std::string EthicsText(const Secret& secret, int today);

// What you are carrying, for a screen that shows it. Empty is the honest
// career and the common one.
std::vector<const Secret*> Unknown(const std::vector<Secret>& secrets);

}  // namespace dirtbag

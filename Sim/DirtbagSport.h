// Sport climbing: the rope, the bolts, and the runout.
//
// `Discipline::Sport` has existed since Phase 0 and did almost nothing — it
// added a mid-route rest stance and was otherwise resolved exactly like a
// six-move boulder. This is the difference.
//
// What actually separates the two, mechanically:
//
//   **Length.** A boulder is 6 to 8 moves and decided by power. A pitch is
//   20 to 40 and decided by pump — which is why the pump bar, the 2D game's
//   best verb, matters more here than anywhere else in the game.
//
//   **You fall on the rope.** The crash pad is irrelevant, so the whole
//   ground-fall penalty has to *stop applying* above the first bolt — it
//   would otherwise punish a roped climber for having no foam under a
//   route they are hanging thirty metres up. In its place is the runout:
//   how far above your last clip you are, which is the sport head game and
//   nothing like the boulder one.
//
//   **Clipping.** Pulling up slack with one hand off a hold is a move you
//   can blow, and blowing it is the classic way to fall on a route you had
//   in the bag. It costs pump, and it costs more from a bad stance.
//
//   **A belayer.** No partner, no sport climbing. The Lot already exists
//   and this is the first thing that genuinely needs it.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagPartner.h"

namespace dirtbag {

struct SportDials {
  // Bolt spacing, in moves. Three is a well-bolted modern sport route: you
  // clip often, you are rarely far above gear, and the head game is mild.
  // The interesting lines are the ones that are not.
  double movesPerBolt = 3.0;

  // The first bolt is high — it always is, and it is why the pad still
  // matters for exactly the first few moves of a pitch and nowhere else.
  int firstBoltAtMove = 2;

  // What a clip costs in pump. A clip from a jug is nearly free; a clip
  // from a crimp with your feet cutting is where routes get lost. Scaled by
  // the stance you are making it from, which the route already describes
  // through restQuality.
  //
  // Sized against basePumpCost (11.0): the first pass had this at 9, which
  // made a clip nearly as expensive as a whole move and took sport's send
  // rate from 65% to zero at every grade. A clip is a fraction of a move,
  // and from the worst stance about half of one.
  double clipPumpCost = 3.0;
  double clipPumpFromBadStance = 2.2;   // multiplier at zero rest quality

  // Being above the bolt, in grade units at maximum runout. Paid out of
  // head the same way a boulder's missing pad is, because it is the same
  // feeling arriving by a different route — and like that one it is worth
  // less to a climber with a good head.
  double runoutGradePenalty = 1.1;

  // How far above the last clip counts as "as bad as it gets". Beyond two
  // bolt-spacings you are not more frightened, you are just further up.
  double runoutSaturationMoves = 6.0;

  // --- The belayer ------------------------------------------------------
  // What somebody will do for you, by how well they know you. A stranger
  // will hold your rope for a couple of laps because that is what people do
  // at a crag; standing under somebody all afternoon while they work the
  // same three moves is a favour, and favours are what rapport is.
  int burnsFromAStranger = 3;
  int burnsAtFullRapport = 12;

  // Below this nobody is unwilling, they are simply not there — the Lot's
  // non-climbers are neighbours, not belayers.
  double minRapportToBelay = 0.0;
};

// Which move each bolt is at, for this route. Derived rather than stored:
// bolts are a property of how a line was equipped, and equipping it the
// same way twice should not need a second copy of the answer.
std::vector<int> BoltsFor(const Route& route,
                          const SportDials& dials = SportDials{});

// The move index of the last clip at or below this move, or -1 when you are
// still below the first bolt — which is the only part of a pitch where the
// ground is what you would hit.
int LastBoltAtOrBelow(const Route& route, int moveIndex,
                      const SportDials& dials = SportDials{});

// Is this a move you clip from?
bool IsClippingMove(const Route& route, int moveIndex,
                    const SportDials& dials = SportDials{});

// 0 clipped and safe .. 1 as far above the bolt as fear goes. Always 0 on a
// boulder, which is what keeps this out of the way of everything already
// measured.
double RunoutAt(const Route& route, int moveIndex,
                const SportDials& dials = SportDials{});

// Extra pump for making the clip from this stance. Zero on a move that is
// not a clip.
double ClipCost(const Route& route, int moveIndex,
                const SportDials& dials = SportDials{});

// Above the first bolt you are on the rope, and the ground is not the
// question any more. This is what stops the crash-pad penalty from
// following a climber up a pitch.
bool OnTheRope(const Route& route, int moveIndex,
               const SportDials& dials = SportDials{});

// --- The belayer -------------------------------------------------------------
//
// No partner, no pitch. This is the first thing in the game that genuinely
// requires the Lot to exist, and it is what makes a rope route a different
// *decision* rather than a longer boulder: a boulder is something you can
// always do alone at dawn, and a pitch is something you have to have
// arranged.
//
// Who will belay you is not a courtesy. Somebody you have never spoken to
// will hold your rope for a lap; somebody you have spent a season with will
// stand there all afternoon while you work the same three moves. That
// difference is rapport, and this is the first place rapport buys something
// you cannot get any other way.

// Will this person tie in with you today? Non-climbers never will, and
// nobody belays a stranger's redpoint burns all afternoon.
// unwired-ok: NOT WIRED -- no partner, no pitch is unenforceable in game.
// Tracked in notes/engine-bridge-gaps.md
bool WillBelay(const Partner& partner, const SportDials& dials = SportDials{});

// How many burns they are good for. A stranger gives you a couple; somebody
// who knows you gives you the day.
// unwired-ok: NOT WIRED -- see WillBelay. Tracked in
// notes/engine-bridge-gaps.md
int BurnsTheyWillHold(const Partner& partner,
                      const SportDials& dials = SportDials{});

// The best belayer among the people at the Lot today, or null if you are
// climbing alone — in which case the rope stays in the van.
// unwired-ok: NOT WIRED -- see WillBelay. Tracked in
// notes/engine-bridge-gaps.md
const Partner* BestBelayer(const std::vector<Partner>& lot,
                           const SportDials& dials = SportDials{});

// Does this route need somebody? Boulders never do.
bool NeedsABelayer(const Route& route);

// "Margo will hold your rope all afternoon" / "nobody is going up there
// with you today"
// unwired-ok: NOT WIRED -- see WillBelay. Tracked in
// notes/engine-bridge-gaps.md
std::string BelayText(const Partner* belayer, const SportDials& dials = SportDials{});

// "bolt 4, and the next one is a long way up"
std::string RunoutText(double runout);

}  // namespace dirtbag

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

// "bolt 4, and the next one is a long way up"
std::string RunoutText(double runout);

}  // namespace dirtbag

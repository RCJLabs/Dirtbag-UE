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

  // **How much a piece you do not trust frightens you**, in runout units,
  // at zero quality. This is the whole of what the runout model needed to
  // learn in order to serve trad as well as sport, and it is why there is
  // one model rather than two: a bolt is protection with quality 1, so
  // this term is exactly zero on every sport route ever resolved, and the
  // golden vectors do not move.
  //
  // At 0.8 a psychological RP with your feet level with it is nearly as
  // frightening as being a full bolt-spacing above a bomber one — which is
  // the honest answer, and it is the sentence a watcher says out loud:
  // *he is not runout, he just doesn't believe in that nut*.
  double poorGearFear = 0.8;

  // **How fast doubt turns into a ground fall**, as the exponent on the
  // slider in FallPenalty below. A shape rather than a magnitude, which is
  // why it lives here and is read straight rather than mirrored.
  //
  // Above 1, and it has to be: a piece you are 70% sure of is not 70% of
  // the way from a bolt to the deck, it is a piece that will almost
  // certainly hold. Held linear, a full rack of cams — which places at
  // about 0.75 in a good crack — priced every runout as though half of the
  // fall would end on the ground, and a 5.12 climber could not lead 5.11.
  // At 2, trust 0.75 costs a little over a bolt, trust 0.45 costs about
  // half way, and trust 0.2 is nearly a solo, which is what those three
  // pieces are.
  double gearDoubtCurve = 2.0;

  // **What nothing at all on the rope costs**, in grade units, once you
  // are properly off the deck.
  //
  // The largest number in the resolver, above pumpGradePenalty (3.0) and
  // well above injuryGradePenalty (2.6), and it should be: it is the only
  // one of them that can kill you. Everything else in this game prices
  // *how hard the move is for you*; this prices what happens if you get it
  // wrong, and forty feet above the deck with nothing in, what happens is
  // not that you get lowered and try again.
  //
  // It exists because the alternative was measured and was a joke. Priced
  // through the crash-pad term — 0.9 grades, sized for a fifteen-foot fall
  // onto foam — a leader who placed nothing sent an eighteen-move pitch
  // 22.6% of the time and one who protected it properly sent 0.3%, because
  // the pump a rack costs is real and the fear it buys back was not.
  // Soloing was not a decision with a downside, it was the answer.
  //
  // Mirrored into SessionDials, which is where the resolver reads it from;
  // this is the owning copy, and TestTheMirroredDialsStillAgree pins the
  // pair. Same arrangement as runoutGradePenalty directly above it.
  double soloGradePenalty = 3.2;

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

// How much you trust what is at exactly this move, 0 for nothing there.
//
// The one place the two disciplines differ, and it is a question about
// where the answer comes from rather than what it means: a sport route
// derives it from the bolts (all of them there, all of them 1), and a trad
// route reads it out of what the leader has managed to get in. Everything
// downstream — the runout, the exposure, the pad cutoff, the head training
// that reads them — is the same model for both.
double PieceAt(const Route& route, int moveIndex,
               const SportDials& dials = SportDials{},
               const Protection& gear = Protection{});

// The move index of the last piece at or below this move, or -1 when you
// are still below the first one — which is the only part of a pitch where
// the ground is what you would hit.
int LastPieceAtOrBelow(const Route& route, int moveIndex,
                       const SportDials& dials = SportDials{},
                       const Protection& gear = Protection{});

// Is this a move you clip from?
bool IsClippingMove(const Route& route, int moveIndex,
                    const SportDials& dials = SportDials{});

// 0 clipped and safe .. 1 as far above the gear as fear goes. Always 0 on
// a boulder, which is what keeps this out of the way of everything already
// measured.
//
// Two things add here, and only one of them exists on a bolted route: how
// far above the last piece you are, and how little you believe in it. On
// sport the second is identically zero and this is the same function it has
// always been.
double RunoutAt(const Route& route, int moveIndex,
                const SportDials& dials = SportDials{},
                const Protection& gear = Protection{});

// Extra pump for making the clip from this stance. Zero on a move that is
// not a clip.
double ClipCost(const Route& route, int moveIndex,
                const SportDials& dials = SportDials{});

// **What a fall from up here costs, in grade units at maximum runout** —
// the runout priced by what is actually going to catch you.
//
// One move on a slider between the two penalties: a bolt or a bomber cam
// costs `runout` and a piece you are certain will rip costs `solo`, because
// a piece you are certain will rip is a solo. Both penalties are passed in
// rather than read off a dial struct, so the resolver can supply its
// mirrored copies and the guidebook can supply the owning ones, and there
// is still only one of this function.
//
// It exists because the fear model saturated at the sport number and could
// not say anything worse than it. Measured with a flat ceiling, a leader
// with a set of nuts sent a pitch **more often** than the same leader with
// a double rack of cams (0.651 against 0.454) — the worse rack made worse
// placements, the bot declined most of them, and running it out was free
// because being above a psychological RP was priced identically to being
// above a bolt. The most expensive purchase in the game made you worse at
// climbing.
double FallPenalty(double trust, double solo, double runout,
                   double curve = 2.0);

// Above the first piece you are on the rope, and the ground is not the
// question any more. This is what stops the crash-pad penalty from
// following a climber up a pitch.
//
// Note what this means on trad, and that it is deliberate: a leader who has
// not placed anything yet is *bouldering*, and the ground-fall model is the
// correct one for them. Getting the first piece in is a real moment.
bool OnTheRope(const Route& route, int moveIndex,
               const SportDials& dials = SportDials{},
               const Protection& gear = Protection{});

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
bool WillBelay(const Partner& partner, const SportDials& dials = SportDials{});

// How many burns they are good for. A stranger gives you a couple; somebody
// who knows you gives you the day.
int BurnsTheyWillHold(const Partner& partner,
                      const SportDials& dials = SportDials{});

// The best belayer among the people at the Lot today, or null if you are
// climbing alone — in which case the rope stays in the van.
const Partner* BestBelayer(const std::vector<Partner>& lot,
                           const SportDials& dials = SportDials{});

// Does this route need somebody? Boulders never do; anything with a rope
// on it does, and a trad second has the worse job of the two.
bool NeedsABelayer(const Route& route);

// "Margo will hold your rope all afternoon" / "nobody is going up there
// with you today"
std::string BelayText(const Partner* belayer, const SportDials& dials = SportDials{});

// "bolt 4, and the next one is a long way up"
std::string RunoutText(double runout);

}  // namespace dirtbag

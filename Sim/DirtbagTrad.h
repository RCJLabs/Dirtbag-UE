// Trad: the gear is not in the rock until you put it there.
//
// Sport arrived in Phase 8 and brought the runout model with it — how far
// above the last clip you are, and what that costs a climber out of head.
// That model is right, and this file does not build a second one. What it
// builds is the other *source* of protection.
//
// The difference between the two disciplines is exactly three things, and a
// watcher can name all of them from the ground:
//
//   **The gear is a decision.** A bolt is where the bolter put it, it is
//   free, and it is bomber. A nut is where the rock lets you and where you
//   choose to spend the pump, and it is only as good as the placement you
//   made under it. Choosing not to place is a real option and sometimes the
//   right one, which is what makes it a decision rather than a checkbox.
//
//   **It costs more than a clip.** Pulling up slack is one hand off a hold
//   for a second. Choosing a size, getting it in, seating it and clipping it
//   is one hand off a hold for considerably longer — and from a bad stance
//   it costs more than the move you are standing on.
//
//   **You can run out of it.** A rack is finite. Sew up the bottom half and
//   the headwall is a solo; save it all and you are twenty feet out for the
//   first half. There is no policy that is right on every pitch, which is
//   the shape a decision is supposed to have.
//
// Everything downstream of that — the runout, the exposure, the pad cutoff
// below the first piece, the head training that reads them — is the sport
// model, unchanged, reading `Protection` instead of `BoltsFor`. That is
// gate 3 of the milestone, and it is why this file has no fear curve in it.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct TradDials {
  // --- What the rock offers ---------------------------------------------
  //
  // Where you can get gear in is a property of what you are holding, and
  // it is derived rather than stored for the same reason the bolts are: a
  // crack takes gear because it is a crack, and a second copy of that fact
  // stored per move would drift from the hold type that already says it.
  //
  // 0 = there is nothing here and there never was; 1 = a hand-sized crack
  // at your waist.
  double gearInACrack = 1.00;
  double gearOnAJug = 0.60;    // a jug is often a slot, a flake, a thread
  double gearOnAPocket = 0.40;
  double gearOnAPinch = 0.25;  // an arête, a rib, sometimes a horizontal
  double gearOnACrimp = 0.15;  // a face edge, and whatever thin thing is near it
  double gearOnASloper = 0.10;
  double gearOnADyno = 0.00;   // you are not placing anything mid-dyno

  // What a stance adds, over and above the hold. A ledge is usually a ledge
  // because something structural is happening, and the thing that gives you
  // a rest very often gives you a placement too.
  double gearFromAStance = 0.25;

  // --- What you manage to get in ----------------------------------------
  //
  // The rock's offer is a ceiling. These take pieces out of it.

  // What a poor rack still gets you, as a fraction. Above zero because a
  // set of nuts in a good crack is a real placement — the rack decides how
  // often the right size is on your harness, not whether gear works.
  double rackFloor = 0.62;

  // Fiddling a cam in with your feet cutting. The single largest thing you
  // control, because it is the one the route already describes: a leader
  // who places from stances and a leader who places where they get scared
  // climb the same pitch and arrive at two different pitches.
  //
  // Read off whichever is better, the stance or the rock — a hand jam is a
  // stance for this purpose even though nothing in the route says so, and
  // an obvious placement is quick wherever you are standing. See
  // PlacingEase in the .cpp for what happened when it was not.
  double stanceHelpsPlacing = 0.35;

  // Rushing it. This is the whole of gate 2 in one number: the same
  // placement, at the same stance, is worse late in the pitch than early —
  // so *when* you spend the rack is a decision made under pump rather than
  // a plan made on the ground.
  double pumpSpoilsPlacing = 0.25;

  // Knowing what goes where. Read off technique, because placing gear is
  // pattern recognition and rope work rather than strength or nerve.
  double craftAtPlacing = 0.22;

  // Below this you did not place a piece, you wasted one — the pump is
  // spent, the piece is gone off the harness, and there is nothing on the
  // rope. The most trad thing in the file.
  double placementFails = 0.18;

  // --- What it costs ----------------------------------------------------
  //
  // Sized against SportDials::clipPumpCost (3.0) and SessionDials::
  // basePumpCost (11.0): a placement is about twice a clip from a jug, and
  // from a bad stance it costs more than the move you are standing on. That
  // is the correct relationship and it is the reason a trad leader climbs
  // to stances rather than to the next scary bit.
  double placePumpCost = 9.0;
  double placeFromBadStance = 2.6;   // multiplier at zero rest quality

  // --- What a sensible leader does --------------------------------------
  //
  // The batch resolver drives the live one with this, exactly the way the
  // bot takes every shake-out it is offered — so the measured game and the
  // played game cannot drift apart. It is also the honest default for a
  // "place gear" prompt: what the game would do if you did not answer.

  // **How much safer it has to make you**, in grade units, before a
  // sensible leader spends the pump and the piece.
  //
  // This one dial replaced two thresholds — "place above this quality" and
  // "place a worse one to get off the deck" — and it replaced them because
  // they were the wrong shape. A quality bar cannot tell a good piece from
  // a *useful* one: measured with a bare 0.24 bar for the first placement,
  // the bot got a nut in at move 1 that it believed in so little the pitch
  // came out more frightening than soloing it (worst exposure 1.06 against
  // 1.05), which is not a leader making a decision, it is a leader with a
  // habit.
  //
  // Asking what the piece *buys* answers both questions at once, and it
  // answers the spacing question too: right above a bomber cam a second
  // one buys nothing and the bot does not place it, without ever being
  // told about spacing.
  //
  // It is a margin on top of what the placement actually costs rather than
  // the whole bar — see gradesAtFullPump below — because "is it worth it"
  // is a comparison and a bare threshold is not.
  double botWorthIt = 0.30;

  // What pump costs in grades, so the leader's arithmetic and the
  // resolver's agree about the price of stopping to fiddle. Mirrors
  // SessionDials::pumpGradePenalty; the number lives there, and
  // TestTheMirroredDialsStillAgree pins the pair.
  //
  // Without it the bot's bar was a bare constant and could not tell a
  // six-pump placement off a jam from a seventeen-pump one off a crimp —
  // so on a crack pitch, where everything is cheap, it sewed the route up
  // to redline and sent it exactly as often as soloing it.
  double gradesAtFullPump = 3.0;

  // Moves between placements when the rock is offering. Under
  // SportDials::movesPerBolt (3.0) would be sewing it up; over it is
  // running it out. Equal to it is the deliberate baseline, because it is
  // what makes "the same rock, bolted or not" a comparison rather than a
  // coincidence.
  double botSpacing = 3.0;

  // How hard to ration. At 1.0 the leader places on pure rock quality and
  // discovers the empty harness at the crux; above it they start saving.
  // The number that makes the rack a budget.
  double botRations = 1.35;

  // --- The shelf --------------------------------------------------------
  //
  // A rack is the most expensive thing in the game, which is true, and it
  // is the only purchase that unlocks a whole discipline rather than
  // improving one. Priced against KitDials::padCost (260) and a season's
  // earnings (~6,600): the nuts are a month of saying no to things and the
  // doubles are most of a year.
  double nutsCost = 190.0;
  double camsCost = 640.0;
  double doublesCost = 1150.0;

  int nutsPieces = 8;
  int camsPieces = 12;
  int doublesPieces = 18;

  double nutsQuality = 0.45;
  double camsQuality = 0.75;
  double doublesQuality = 0.95;
};

// --- The rock's offer --------------------------------------------------------

bool IsTrad(const Route& route);

// 0..1: how much gear this move's rock will take, before you have anything
// to do with it. Derived from the hold and the stance, never stored.
double TakesGear(const Move& move, const TradDials& dials = TradDials{});

// --- The placement -----------------------------------------------------------

// What you actually get in, here, now, at this pump, with this rack — 0
// when the answer is "nothing worth having", which still costs you the
// piece and the pump at the call site that decided to try.
//
// Pure, and deliberately so: it is the thing gate 2 is about, and a
// function of the stance and the pump can be swept without staging an
// attempt.
double PlaceHere(const Move& stance, double pump, const Climber& climber,
                 const Rack& rack, const TradDials& dials = TradDials{});

// What trying costs in pump, whether or not it works. Cheap from a stance
// or into an obvious crack, and from a blank wall with no feet it costs
// more than the move you are standing on.
double PlaceCost(const Move& stance, const TradDials& dials = TradDials{});

// What a sensible leader does here — the batch bot's policy, and the
// default answer to the live prompt. Reads the rock ahead, the gear
// already on the rope, and how much rack is left against how much pitch
// is left.
bool WorthPlacing(const Route& route, int moveIndex, const Protection& gear,
                  const Rack& left, double pump, const Climber& climber,
                  const TradDials& dials = TradDials{});

// --- The rack ----------------------------------------------------------------

// What is on the shelf, in the order a dirtbag buys it. Nobody's first rack
// is a set of cams.
enum class RackTier { None = 0, Nuts, Cams, Doubles };
constexpr int kRackTierCount = 4;

RackTier TierOf(const Rack& rack, const TradDials& dials = TradDials{});
Rack RackOf(RackTier tier, const TradDials& dials = TradDials{});
double RackPrice(RackTier tier, const TradDials& dials = TradDials{});
const char* RackTierName(RackTier tier);

// Buys the next tier up. False and unchanged when you cannot afford it or
// there is nothing above what you have — the caller never has to unwind a
// half-purchase, same contract as the kit shop. `priceMult` is what the
// counter charges *this* climber.
bool BuyRack(Rack& rack, double& cash, const TradDials& dials = TradDials{},
             double priceMult = 1.0);

// No rack, no lead. The rope stays in the van for a different reason than
// it does when nobody will belay you, and both of them are real.
bool CanLeadTrad(const Rack& rack);

// --- Words -------------------------------------------------------------------

// "bomber" / "it'll do" / "psychological". Never a number: the whole point
// of a placement is that you are looking at it and deciding.
const char* PieceText(double quality);

// "a rack of cams, eleven pieces left"
std::string RackText(const Rack& rack, const TradDials& dials = TradDials{});

}  // namespace dirtbag

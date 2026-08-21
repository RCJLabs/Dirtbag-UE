// The kit — things worth money that buy you climbing.
//
// This exists because of a measurement, not a wishlist. Phase 3's gate is
// that the money loop should pressure the climbing loop, and measured
// honestly against a control player with no bills at all, it did not: a
// season of paying rent cost 0 climbing days, 2 burns and no sends
// (notes/phase3-economy.md). The reason was not that money was too easy. It
// was that there was nothing to want. The only discretionary spending in a
// whole year was shoes — $275 against $6,601 earned — and the year ended
// with $133 in the bank and nowhere for it to go. Money cannot pressure
// climbing while money does not buy climbing.
//
// So: three things to want, each doing a different job.
//
//   crash pads      remove a penalty you are otherwise always paying
//   a hangboard     the broke answer: training on a day you cannot climb
//   a gym membership the only climbing that ignores the weather
//
// The membership is the load-bearing one. A season has 157 days that never
// come good and 75 more spent resting skin, and every one of them is
// currently dead time no amount of money can touch. A membership converts
// them, and it is the first recurring cost in the game — the first thing
// that keeps mattering after you have bought it.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct KitDials {
  // --- Pads -------------------------------------------------------------
  // Landing under you. Without one you are climbing above gravel and it
  // shows exactly where you are trying hardest: high on a limit line.
  //
  // Two is the number that matters. One pad covers the fall you expected;
  // the second covers the one you did not, which is the whole argument for
  // owning it. Past two the returns are real but small, and a dirtbag
  // borrows the third from whoever is at the Lot.
  double padCost = 260.0;
  int padsThatMatter = 2;

  // The most foam can ever do for you, 0..1. Deliberately under 1.0.
  //
  // At 1.0 a second pad put padding at exactly 1.0, exposure at exactly 0,
  // and head training at exactly zero — permanently, since head never
  // declines either. Measured: +0.00 head in eleven of twelve seasons. The
  // trade read "about one send a year in exchange for every point of head
  // you will ever have", which is a switch rather than a decision.
  //
  // Below 1.0 it is a trade again, and it is also just true: a well-padded
  // highball is still a highball. You can cover the landing and you cannot
  // make the ground stop being down there.
  //
  // 0.85 is the knee, swept across twelve seasons of a player who buys the
  // second pad. Head over a year: 1.00 -> +0.01, 0.90 -> +0.41,
  // **0.85 -> +0.58**, 0.80 -> +0.78, 0.75 -> +0.99. Sends hold at a median
  // of 5.0 all the way down to 0.85 and then fall off a cliff to 3.5, so
  // anything below this buys head by making the pad not worth owning, which
  // is not a trade, it is the same switch pointing the other way.
  //
  // At 0.85 a two-pad season trains about a third of the head a one-pad
  // season does (+0.58 against +1.78) and keeps every send. That is the
  // shape it should have been all along.
  double mostFoamCanDo = 0.85;

  // What climbing unpadded costs, in grade units, at the top of a line.
  // Sized against deadShoeGradePenalty (1.1): being scared is a real
  // handicap and never the whole story. It scales with height, so the first
  // moves are free and the last ones are not — which is where a boulderer
  // actually backs off, and why the pad is worth its price.
  // Mirrored into SessionDials, which is where the resolver reads them
  // from; these are the owning copies. Same arrangement as GearDials for
  // shoes and SportDials for the runout — share the function, mirror the
  // constant — and TestTheMirroredDialsStillAgree pins the pair, because
  // until it did, both of these were tunable here with no effect anywhere.
  double noPadGradePenalty = 0.9;

  // Below this fraction of the way up, the ground is close enough that no
  // amount of foam is the point. Nobody has ever been gripped on move one.
  double padGroundedFraction = 0.35;

  // --- The hangboard ----------------------------------------------------
  // Eighty-five dollars of plywood and a screw gun. It is the cheap answer
  // to a washed-out day, and deliberately worse than climbing: it trains
  // one thing, it costs skin, and it teaches you nothing about movement.
  double hangboardCost = 85.0;
  double hangboardHours = 1.0;
  // Hanging on wood with chalk on takes almost no skin off — that was never
  // what a board costs, and pricing it in skin was a stand-in for a budget
  // that did not exist yet. What a hangboard actually costs is tendons.
  double hangboardSkinCost = 0.3;

  // And that is the real price: more than a four-burn session, on the body's
  // slow budget. A hangboard is the single most reliable way a climber gets
  // injured, and until training load existed the board accrued *nothing* —
  // it was free training with no way to pay for it, which is why a probe
  // that owned one used it 177 days a year.
  double hangboardLoad = 14.0;
  // Fingers only, and modest. Measured against the wall: a four-burn
  // session on a hard line trains 0.15 of a finger point, so 0.06 puts the
  // board at about 40% of a light session — worth doing on a day the
  // weather has already taken, never a substitute for one.
  //
  // The first pass at this had it at 0.22, and because nothing stopped you
  // hanging repeatedly it delivered 1.44 in a day against climbing's 0.32.
  // A board four times better than rock is not a training aid, it is a
  // reason never to leave the van.
  double hangboardFingerGain = 0.06;
  double hangboardEnergy = 18.0;

  // --- The gym ----------------------------------------------------------
  // A month at a time, because that is how gyms sell it and because a
  // recurring bill is the point: it is the first thing you can lose by
  // being broke in a way you notice.
  double membershipCost = 75.0;
  int membershipDays = 30;

  // The drive in, each way. Plastic is never quite as convenient as the
  // rock you are parked under.
  double gymTravelHours = 0.5;
};

// What you own. Small on purpose — a kit list that needs a manager is a
// shop, and this is a van.
struct Kit {
  // You arrive with one. Nobody drives to a boulder with no pad, and
  // starting at zero is not the fantasy — measured that way, adding pads
  // took the five-seed send count from 17 to 3, which is not a new decision
  // for the player, it is the game quietly getting harder. One pad covers
  // the fall you expected; the second is the purchase, and it is the one
  // that covers the fall you did not.
  int pads = 1;
  bool hangboard = false;

  // Days of gym membership left. Zero is not a member.
  int membershipDaysLeft = 0;
};

// Buying. Each returns false if you cannot afford it, and changes nothing
// when it does — the caller never has to unwind a half-purchase.
bool BuyPad(Kit& kit, double& cash, const KitDials& dials = KitDials{});
bool BuyHangboard(Kit& kit, double& cash, const KitDials& dials = KitDials{});
bool RenewMembership(Kit& kit, double& cash, const KitDials& dials = KitDials{});

// Lapses at midnight like everything else that runs out.
void KitDay(Kit& kit);

bool IsGymMember(const Kit& kit);

// How covered the landing is, 0 bare ground .. 1 as padded as it gets.
// Feeds AttemptInput::padding, which is why it is a scalar and not a count.
double PaddingFrom(const Kit& kit, const KitDials& dials = KitDials{});

// What the kit is worth saying out loud, in the game's register.
std::string KitText(const Kit& kit);

// What the pad you do not own would do for you, and what it would cost you
// that is not money. Empty once you have the pads that matter — the third
// is borrowed from whoever is at the Lot.
//
// The second half of that sentence is the point. The pad is the single
// purchase measured to move a season most, and its price in head was
// invisible: a player found out months later that they had stopped getting
// braver, with nothing ever having said so.
std::string PadOfferText(const Kit& kit, const KitDials& dials = KitDials{});

}  // namespace dirtbag

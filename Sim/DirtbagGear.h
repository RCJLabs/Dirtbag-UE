// Gear, which is where money finally touches climbing.
//
// Phase 3's gate is that the money loop should pressure the climbing loop.
// Measured before building any of it, a year cost about $5,100 against
// $4,860 earned from 81 shifts — 22% of days worked, cash never below $93,
// no risk at any point (notes/phase3-economy.md). Bills are a flat drip
// that has nothing to do with climbing, so working more simply solves them
// and the two loops never meet.
//
// Shoes are the coupling. They wear by the move, so the cost scales with
// exactly the thing the player wants to do more of: climb hard, wear them
// out, work to replace them, lose the days you were going to climb. And
// dead rubber is not merely expensive — it costs grades, so the trap has
// teeth before it has a price tag.
//
// The decision is the dirtbag one: resole cheap and keep a pair alive, or
// admit the uppers are finished and find $165.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct GearDials {
  // A pair of shoes, in moves. Real rubber lasts a few months of steady
  // climbing; 700 moves is roughly a season at the probe's rate of ~1,200
  // moves a year, so a committed climber buys two or three pairs a year and
  // a casual one buys fewer. Wear scales with how hard you pull, because
  // that is what eats toe rubber.
  double shoeLifeMoves = 700.0;
  double wearPerGradeOverFive = 0.12;   // hard climbing eats rubber faster

  // What dead rubber costs. Sized under coldStartPenalty (1.5) — shoes are
  // a real handicap and never the whole story — and it lands hardest on the
  // holds that need edging.
  double deadShoeGradePenalty = 1.1;
  double deadShoeBiteOnGoodHolds = 0.3;

  // The money. A resole is most of a new pair's performance for a third of
  // the price, but the uppers only take so many before there is nothing
  // left to glue rubber to.
  double resoleCost = 55.0;
  double newShoeCost = 165.0;
  int resolesPerPair = 2;
  double resoleRestores = 0.7;   // a resole is not a new shoe

  // Below this the rubber is done enough to notice.
  double noticeablyWorn = 0.55;
};

struct Shoes {
  double wear = 0.0;    // 0 new .. 1 dead
  int resoles = 0;      // how many this pair has had
  int pairsOwned = 1;   // lifetime, for the career line
};

// Climb on them. `moves` is what you actually did, `grade` how hard.
void WearShoes(Shoes& shoes, int moves, int grade,
               const GearDials& dials = GearDials{});

// Grades lost on this kind of hold, right now. The shop's way in.
double ShoePenalty(const Shoes& shoes, bool edgingHold,
                   const GearDials& dials = GearDials{});

// The one place dead rubber is priced, taking the two numbers rather than a
// dial struct so the resolver can call it without dragging GearDials
// through DirtbagSession.
//
// This project's standing arrangement for a value two systems both need is
// *share the function, mirror the constant* — the runout does it, the crash
// pad does it. Shoes were doing neither: SessionDials mirrored the numbers,
// as it should, and then the resolver wrote the formula out again beside
// them. `ShoePenalty` ended up with no caller at all, which is how the
// duplication surfaced at all. This pins the arithmetic; the test pins the
// numbers.
double ShoePenaltyFor(double wear, bool edgingHold, double deadPenalty,
                      double biteOnGoodHolds);

// Can this pair take another resole, and what would it cost?
bool CanResole(const Shoes& shoes, const GearDials& dials = GearDials{});

// Returns false if you cannot afford it, or the uppers are finished.
//
// `sponsored` is who is paying, and it has no default on purpose. A shoe
// deal is the entire content of the bottom sponsorship rung — "free shoes,
// and they want nothing" — and `CoversShoes` sat written and uncalled from
// the day it landed, so that rung gave the player precisely nothing. A
// parameter every caller has to answer is the only version of this that
// cannot go quiet again.
bool Resole(Shoes& shoes, double& cash, bool sponsored,
            const GearDials& dials = GearDials{});
bool BuyNewShoes(Shoes& shoes, double& cash, bool sponsored,
                 const GearDials& dials = GearDials{});

// "the rubber is going" / "you can see your toes"
std::string ShoeText(const Shoes& shoes, const GearDials& dials = GearDials{});

}  // namespace dirtbag

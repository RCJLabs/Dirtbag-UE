#pragma once

// The campfire game.
//
// `concepts/DIRTBAG.md` section 4 cuts the 2D game's minigames down to
// *"keep ONE campfire game"* and names poker as the example. This is that
// one, and the rules below are a design rather than a port -- the repo has
// the sentence and nothing else.
//
// **Why the fire needed a verb.** SitAtTheFire already gives rapport,
// psyche and a line of talk for spending hours there. It is entirely
// passive: you sit, you receive, there is no decision and nothing you can
// be bad at. Meanwhile 5,056 days of a thirty-year career never come good
// (notes/dreams-what-money-is-worth.md) and the evening of a washed-out day
// has nothing in it.
//
// **Why money is the stake.** Until dreams existed, cash had no
// destination, so losing some was a rounding error -- the 2D game's poker
// would have been flavour. Now the float is what a dream costs *and* what
// keeps the van off the bodge, so a bad night at the fire is felt twice.
// The campfire game got its teeth from a system built three days after it
// was cut.
//
// **Why rapport is the skill.** You are not reading cards, you are reading
// people. How well you know somebody is how well you see what they have --
// so the social system carries the minigame instead of a card simulator
// bolted onto the side, and the reason to sit at this fire for a season is
// mechanical rather than sentimental.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagPartner.h"
#include "DirtbagRng.h"

namespace dirtbag {

struct CampfireDials {
  // Dirtbags do not play for real money. The ante is a beer and the ceiling
  // is a tank of fuel -- enough that a bad run costs you a week of the
  // float, never enough that one night ends a dream.
  double ante = 5.0;
  double maxStake = 40.0;

  // How well you read somebody at no rapport and at full. Neither end is
  // absolute: a stranger is not blind luck and a friend of ten years still
  // gets you now and then, which is what keeps it a game rather than a
  // lookup once the Lot likes you.
  double readAtNoRapport = 0.15;
  double readAtFullRapport = 0.85;

  // What the rest of the table needs before it will match a raise. This
  // number is the whole balance of the game and it was found by measuring:
  // with nobody folding, a player who used the read won **$14 a hand at
  // zero rapport**, because folding cost the ante and winning was paid by
  // three people who always called. The fire would have been an infinite
  // cash machine bolted to the side of an economy where money buys van
  // uptime and dreams.
  //
  // Now the Lot folds what it would fold. Raise on a monster and you win
  // the antes and nothing more, which is exactly what happens in real life
  // when you bet big with the nuts.
  double theyStayAbove = 0.45;

  // An evening of cards is worth about an eighth of a day of climbing
  // together, which is the honest exchange rate: it is company, not a rope.
  double rapportPerHand = 0.01;

  // Winning is a good night and losing is a night. Asymmetric on purpose --
  // the point of the game is not to be a psyche pump.
  double psychePerWin = 0.03;
  double psychePerLoss = 0.02;
};

// What you are looking at. `reads` is what you *think* each of them has,
// which is the true value blurred by how little you know them -- never the
// truth, and never pure noise.
struct CampfireHand {
  double yours = 0.0;                 // 0..1, and you see this exactly
  std::vector<std::string> who;
  std::vector<double> reads;          // 0..1, what you think they have
  std::vector<double> truth;          // 0..1, what they actually have
  double pot = 0.0;                   // the antes, already in
};

struct CampfireResult {
  bool folded = false;
  bool won = false;
  double cashDelta = 0.0;             // signed, including the ante
  std::string beat;                   // who had the best of the rest
  std::string line;                   // what gets said
};

// Deal one hand. Deterministic per world, day and hand number, so an
// evening replays identically and reloading cannot reroll a bad night.
CampfireHand DealHand(const std::vector<Partner>& lot, const Rng& worldRng,
                      int day, int handNumber,
                      const CampfireDials& dials = CampfireDials{});

// Play it. `stake` is what you put in on top of the ante; folding forfeits
// the ante and nothing else. Applies cash, psyche and rapport -- the whole
// hand resolves here so a caller cannot take the winnings and skip the
// social half.
CampfireResult PlayHand(const CampfireHand& hand, double& cash,
                        double& psyche, std::vector<Partner>& lot,
                        double stake, bool fold,
                        const CampfireDials& dials = CampfireDials{});

// "Margo is not even looking at her cards." What a read looks like in
// words, which is all the player should ever see of a number.
std::string ReadText(double read);

}  // namespace dirtbag

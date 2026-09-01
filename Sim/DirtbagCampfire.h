#pragma once

// The campfire games -- all three of them.
//
// `concepts/DIRTBAG.md` section 4 cut these to *"keep ONE campfire game"*.
// Evan reversed that on 2026-08-22 (`concepts/PIVOT-campfire-games.md`):
// liar's dice, poker and blackjack all ship, because they already exist in
// the 2D game, they are already balanced, and they are the one category on
// that cut list which is pure sim -- no art, no animation, no new places.
//
// They are deliberately not one game three times. Each asks something
// different, and blackjack earns its place by asking nothing social at all:
//
//   poker        read the person       -- rapport IS the skill
//   liar's dice  is he lying, and dare you say so
//                                      -- rapport sharpens the tell
//   blackjack    one decision, no people
//                                      -- rapport does nothing
//
// That last is the game for a climber who has just arrived somewhere and
// knows nobody, which is a real state in this game and one that nothing
// else pays off.
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

  // How much better a hand the table needs before it will match a *big*
  // raise, on top of `theyStayAbove` and scaled by how much of the ceiling
  // you shoved.
  //
  // Added when the fire got a table and the stake became something the
  // player chooses (2026-08-22). Measured first, and the measurement was
  // damning: with the threshold flat, the Lot called a $40 shove exactly as
  // often as a $5 nudge, so the stake scaled the winnings **linearly and
  // never flipped sign** -- +$3.35 a hand at the ceiling against +$0.01 at
  // the ante, at every rapport from stranger to friend. A control whose
  // only correct setting is "maximum" is not a decision, it is a lever with
  // one end.
  //
  // The dial comment above already said what should happen -- *"a big raise
  // into a weak table wins the antes and nothing else"* -- and it did not,
  // because nothing read the size of the raise. Now it does: shove and they
  // fold, so you win the middle and no more; nudge and they call, so a good
  // hand gets paid. Downside is untouched either way, because losing costs
  // what you put in whatever the table thought of it.
  double shoveMakesThemFold = 0.30;

  // An evening of cards is worth about an eighth of a day of climbing
  // together, which is the honest exchange rate: it is company, not a rope.
  double rapportPerHand = 0.01;

  // --- Liar's dice ---
  int diceEach = 5;
  // How greedy the table's bids are, as a fraction of the expected count.
  //
  // Found by measuring, and the first value was badly wrong. At 1.35 the
  // bid sat well above what the dice actually held, so **almost every bid
  // was a lie and calling blindly won $19 a round** -- which also made the
  // tell worth *less* than not thinking, because a player who only called
  // on a tell passed up all the free money.
  //
  // A liar's dice bid has to be true more often than not. That is what
  // makes a bluff a bluff and what makes calling a decision instead of a
  // reflex.
  double bidAmbition = 0.85;

  // --- Blackjack ---
  // The dealer stands on this and everything above it. Seventeen because
  // that is what a dealer does, and because it is what makes hitting on
  // fifteen a decision rather than a mistake.
  int dealerStandsOn = 17;
  // Even money. An earlier 1.2 -- meant to stop the house edge making this
  // the worst seat at the fire -- combined with ties going to the player to
  // make blackjack **profitable at every sane strategy**, which is not a
  // game, it is an ATM. Ties push instead now, and the pay is flat.
  //
  // 1.05 rather than 1.0: at flat pay, best play lost $5.91 a hand against
  // the $5 ante, so the cards themselves were costing you money on top of
  // the seat. The intent is that **good blackjack costs you the ante and
  // nothing more** -- it is the game with no edge to build, not the game
  // that punishes you for sitting down.
  double blackjackPays = 1.05;

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
CampfireHand DealPoker(const std::vector<Partner>& lot, const Rng& worldRng,
                       int day, int handNumber,
                       const CampfireDials& dials = CampfireDials{});

// Play it. `stake` is what you put in on top of the ante; folding forfeits
// the ante and nothing else. Applies cash, psyche and rapport -- the whole
// hand resolves here so a caller cannot take the winnings and skip the
// social half.
CampfireResult PlayPoker(const CampfireHand& hand, double& cash,
                         double& psyche, std::vector<Partner>& lot,
                         double stake, bool fold,
                         const CampfireDials& dials = CampfireDials{});

// --- Liar's dice -------------------------------------------------------------
//
// Five dice each, under a cup. Somebody bids that the table holds at least
// N dice showing a given face, counting ones as wild. The bid comes round to
// you and you have exactly one decision: **call it, or pass it on**.
//
// Where poker asks what somebody has, this asks whether they are lying --
// so rapport buys you a tell on the *bidder* rather than a read on their
// hand, and the rest is nerve. Calling a true bid costs you; calling a
// bluff pays.

struct LiarsDiceRound {
  std::vector<int> yours;        // your five, which you see
  std::string bidder;            // whose bid it is
  int bidCount = 0;              // "there are at least N of these"
  int bidFace = 0;               // 2..6; ones are wild and are never bid
  int actual = 0;                // the truth, across the table
  double tell = 0.0;             // 0..1, how much they look like lying
  int diceOnTable = 0;
};

LiarsDiceRound DealLiarsDice(const std::vector<Partner>& lot,
                             const Rng& worldRng, int day, int roundNumber,
                             const CampfireDials& dials = CampfireDials{});

// `call` challenges the bid. Passing accepts it and pushes the decision on,
// which is safe and wins nothing -- the ante still goes.
CampfireResult PlayLiarsDice(const LiarsDiceRound& round, double& cash,
                             double& psyche, std::vector<Partner>& lot,
                             double stake, bool call,
                             const CampfireDials& dials = CampfireDials{});

// "Dev will not put the cup down." What a tell looks like in words.
std::string TellText(double tell);

// --- Blackjack ---------------------------------------------------------------
//
// The one with nobody in it. You against the deck, the Lot watching. No
// read, no rapport, no bluff -- just whether you take another card, which
// is the whole game and is a real decision under a known distribution.
//
// It is here for the climber who has just turned up and knows nobody. Every
// other thing at this fire is gated on people.

struct BlackjackHand {
  int yours = 0;                 // your total
  int dealerShows = 0;           // the one card you can see
  bool bust = false;
  bool finished = false;
  int draws = 0;                 // how many you have taken
};

BlackjackHand DealBlackjack(const Rng& worldRng, int day, int handNumber);

// Take another. Mutates the hand; sets `bust` and `finished` if it goes
// over. Returns what you drew, or 0 if the hand was already done.
int Hit(BlackjackHand& hand, const Rng& worldRng, int day, int handNumber);

// Stop, and settle. The dealer plays itself out to 17 the way a dealer
// does, which is the only reason this is a game and not a coin toss.
CampfireResult Stand(BlackjackHand& hand, double& cash, double& psyche,
                     std::vector<Partner>& lot, double stake,
                     const Rng& worldRng, int day, int handNumber,
                     const CampfireDials& dials = CampfireDials{});

// "Margo is not even looking at her cards." What a read looks like in
// words, which is all the player should ever see of a number.
std::string ReadText(double read);

// --- What is out tonight -----------------------------------------------------
//
// You join what is being played rather than ordering off a menu, so which
// game is out is a fact about the day rather than a choice the player
// makes. That makes it a *rule*, and rules live here.
//
// It lived in the presentation layer until 2026-08-22, recomputed as
// `day % 3` in four separate places -- the fire's prompt, the deal, the
// commit and the back-down. Four copies of one rule is three chances for
// two of them to disagree about which game they are settling, and the
// engine has no test that could ever notice.
enum class FiresideGame { Cards = 0, Dice = 1, Blackjack = 2 };
constexpr int kFiresideGameCount = 3;

FiresideGame WhatsOutTonight(int day);

// "cards", "liar's dice", "blackjack". Lower case because it goes into the
// middle of a sentence: "cards out tonight".
const char* GameName(FiresideGame game);

// --- What you put in ---------------------------------------------------------
//
// Three stakes, cheapest first: the ante, half the ceiling, the ceiling.
// The engine used to carry one hand-typed 20.0 and no way to change it, so
// a player could not bet small on a bad hand -- which removes the only
// decision poker has that is not "fold".
//
// Derived from the dials rather than typed into the engine, so retuning
// `maxStake` moves what the keys do instead of leaving the top notch
// quietly below the ceiling it is meant to be.
constexpr int kStakeNotches = 3;
double StakeNotch(int notch, const CampfireDials& dials = CampfireDials{});

}  // namespace dirtbag

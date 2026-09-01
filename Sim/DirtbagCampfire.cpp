#include "DirtbagCampfire.h"

#include <algorithm>

namespace dirtbag {

namespace {

double Clamp01In(double v) { return std::max(0.0, std::min(1.0, v)); }

// How clearly you see somebody, from how well you know them.
double ReadQuality(double rapport, const CampfireDials& dials) {
  const double r = Clamp01In(rapport);
  return dials.readAtNoRapport +
         (dials.readAtFullRapport - dials.readAtNoRapport) * r;
}

}  // namespace

CampfireHand DealPoker(const std::vector<Partner>& lot, const Rng& worldRng,
                      int day, int handNumber, const CampfireDials& dials) {
  CampfireHand hand;
  // Its own stream, keyed on the hand: an evening replays identically, and
  // reloading cannot reroll a night that went badly.
  Rng rng = worldRng.Derive("campfire#" + std::to_string(day) + "#" +
                            std::to_string(handNumber));

  hand.yours = rng.NextDouble();
  hand.pot = dials.ante;   // yours

  for (const Partner& p : lot) {
    const double truth = rng.NextDouble();
    hand.who.push_back(p.name);
    hand.truth.push_back(truth);
    hand.pot += dials.ante;

    // The read: the truth, pulled towards a coin-flip by how little you
    // know them. At full rapport you are looking almost straight at it; at
    // none you are mostly looking at 0.5, which is the honest shape of
    // having no idea -- it does not lie to you, it just tells you nothing.
    const double q = ReadQuality(p.rapport, dials);
    const double noise = rng.NextDouble();
    const double blurred = truth * q + noise * (1.0 - q);
    hand.reads.push_back(Clamp01In(blurred));
  }
  return hand;
}

CampfireResult PlayPoker(const CampfireHand& hand, double& cash,
                        double& psyche, std::vector<Partner>& lot,
                        double stake, bool fold, const CampfireDials& dials) {
  CampfireResult out;

  // An evening of cards is company however it goes, so rapport is paid
  // first and paid whatever happens. Folding is still sitting there.
  for (Partner& p : lot) {
    p.rapport = std::min(1.0, p.rapport + dials.rapportPerHand);
  }

  if (fold) {
    out.folded = true;
    out.cashDelta = -dials.ante;
    cash = std::max(0.0, cash - dials.ante);
    out.line = "You throw them in.";
    return out;
  }

  // You cannot bet what you do not have, and you cannot bet the farm --
  // the ceiling is the dial, not your nerve.
  const double put =
      std::max(0.0, std::min(std::min(stake, dials.maxStake),
                             std::max(0.0, cash - dials.ante)));

  // Who is still in. The Lot folds what it would fold -- see
  // `theyStayAbove`. A big raise into a weak table wins the antes and
  // nothing else, which is what betting big with the nuts actually gets
  // you.
  int stayed = 0;
  double best = -1.0;
  std::size_t bestAt = 0;
  bool anyIn = false;
  //
  // What they need scales with what you shoved: the same hand that calls a
  // nudge lays down against the ceiling.
  const double demand =
      std::min(0.99, dials.theyStayAbove +
                         dials.shoveMakesThemFold *
                             (dials.maxStake > 0.0 ? put / dials.maxStake
                                                   : 0.0));
  for (std::size_t i = 0; i < hand.truth.size(); i++) {
    if (hand.truth[i] < demand) continue;
    stayed++;
    anyIn = true;
    if (hand.truth[i] > best) { best = hand.truth[i]; bestAt = i; }
  }
  if (anyIn && bestAt < hand.who.size()) out.beat = hand.who[bestAt];

  // Everyone folded: you win what is in the middle and learn nothing.
  out.won = !anyIn || hand.yours >= best;
  if (out.won) {
    // The pot, plus what the players who stayed matched of your raise.
    const double matched = put * static_cast<double>(stayed);
    out.cashDelta = (hand.pot - dials.ante) + matched;
    cash += out.cashDelta;
    psyche = std::min(1.0, psyche + dials.psychePerWin);
    out.line = out.beat.empty()
                   ? "You take it."
                   : "You take it. " + out.beat + " wanted that one.";
  } else {
    out.cashDelta = -(dials.ante + put);
    cash = std::max(0.0, cash + out.cashDelta);
    psyche = std::max(0.05, psyche - dials.psychePerLoss);
    out.line = out.beat + " had it all along.";
  }
  return out;
}

// --- Liar's dice -------------------------------------------------------------

LiarsDiceRound DealLiarsDice(const std::vector<Partner>& lot,
                             const Rng& worldRng, int day, int roundNumber,
                             const CampfireDials& dials) {
  LiarsDiceRound out;
  Rng rng = worldRng.Derive("dice#" + std::to_string(day) + "#" +
                            std::to_string(roundNumber));

  const int players = static_cast<int>(lot.size()) + 1;
  out.diceOnTable = players * dials.diceEach;

  // Everybody's dice, yours first. Ones are wild, so a face's true count is
  // its own plus the ones -- which is why nobody ever bids ones.
  out.bidFace = rng.IntRange(2, 6);
  int wild = 0, matching = 0;
  for (int p = 0; p < players; p++) {
    for (int d = 0; d < dials.diceEach; d++) {
      const int pip = rng.IntRange(1, 6);
      if (p == 0) out.yours.push_back(pip);
      if (pip == 1) wild++;
      else if (pip == out.bidFace) matching++;
    }
  }
  out.actual = matching + wild;

  if (lot.empty()) return out;   // nobody to bid; a legal but dead round

  // Whose bid, and how greedy. The expected count for a face with wilds is
  // a third of the table, so ambition above 1.0 is how often the bid is a
  // lie -- and the lie is the whole game.
  const std::size_t who = static_cast<std::size_t>(
      rng.IntRange(0, static_cast<int>(lot.size()) - 1));
  out.bidder = lot[who].name;

  const double expected = out.diceOnTable / 3.0;
  const double greed = dials.bidAmbition * (0.7 + 0.6 * rng.NextDouble());
  out.bidCount = std::max(1, static_cast<int>(expected * greed + 0.5));

  // The tell: whether the bid is a lie, blurred by how little you know
  // them. Deliberately *not* the poker read's formula, and the difference
  // matters. `truth*q + noise*(1-q)` works for a value estimate but the
  // thing being estimated here is binary, and at any q >= 0.5 the two cases
  // stop overlapping -- a lie lands above 0.5 and a true bid below it,
  // every single time. Measured: rapport 0.5 and rapport 1.0 scored
  // identically to the cent, because both were **perfect lie detectors**,
  // which makes a liar's-dice table pointless the moment anybody likes you.
  //
  // Symmetric noise around a signal instead, so the ranges always overlap:
  // at full rapport a liar reads 0.45..1.05 and an honest bid -0.05..0.55,
  // and the sliver in the middle is a friend of ten years still getting you
  // now and then.
  const bool lying = out.bidCount > out.actual;
  const double q = ReadQuality(lot[who].rapport, dials);
  const double base = lying ? 0.75 : 0.25;
  out.tell = Clamp01In(base + (rng.NextDouble() - 0.5) * 2.0 * (1.0 - q));
  return out;
}

CampfireResult PlayLiarsDice(const LiarsDiceRound& round, double& cash,
                             double& psyche, std::vector<Partner>& lot,
                             double stake, bool call,
                             const CampfireDials& dials) {
  CampfireResult out;
  for (Partner& p : lot) {
    p.rapport = std::min(1.0, p.rapport + dials.rapportPerHand);
  }
  out.beat = round.bidder;

  if (!call) {
    // Passing it on is safe and wins nothing. The ante still goes, because
    // sitting at the table costs whether or not you do anything at it.
    out.folded = true;
    out.cashDelta = -dials.ante;
    cash = std::max(0.0, cash - dials.ante);
    out.line = "You let it go round.";
    return out;
  }

  const double put =
      std::max(0.0, std::min(std::min(stake, dials.maxStake),
                             std::max(0.0, cash - dials.ante)));

  // The count settles it. Calling a bluff wins; calling a true bid pays the
  // bidder, which is what stops calling everything.
  out.won = round.bidCount > round.actual;
  if (out.won) {
    out.cashDelta = put;
    cash += out.cashDelta;
    psyche = std::min(1.0, psyche + dials.psychePerWin);
    out.line = round.bidder + " had " + std::to_string(round.actual) + ".";
  } else {
    out.cashDelta = -(dials.ante + put);
    cash = std::max(0.0, cash + out.cashDelta);
    psyche = std::max(0.05, psyche - dials.psychePerLoss);
    out.line = "There were " + std::to_string(round.actual) + ". " +
               round.bidder + " counts them out slowly.";
  }
  return out;
}

std::string TellText(double tell) {
  if (tell < 0.2) return "said it like a fact";
  if (tell < 0.4) return "did not look up";
  if (tell < 0.6) return "is giving nothing away";
  if (tell < 0.8) return "took a moment too long";
  return "will not put the cup down";
}

// --- Blackjack ---------------------------------------------------------------

namespace {

// Cards, not a shoe. A campfire deck is shuffled every hand and nobody is
// counting -- modelling a shoe would make card counting the skill, and the
// point of this one is that it has no skill you can build, only a decision
// you can get right.
int DrawCard(Rng& rng) {
  const int c = rng.IntRange(1, 13);
  return c > 10 ? 10 : c;   // faces are ten; aces are one, and stay one
}

}  // namespace

BlackjackHand DealBlackjack(const Rng& worldRng, int day, int handNumber) {
  BlackjackHand hand;
  Rng rng = worldRng.Derive("jack#" + std::to_string(day) + "#" +
                            std::to_string(handNumber));
  hand.yours = DrawCard(rng) + DrawCard(rng);
  hand.dealerShows = DrawCard(rng);
  return hand;
}

int Hit(BlackjackHand& hand, const Rng& worldRng, int day, int handNumber) {
  if (hand.finished) return 0;
  // Keyed on the draw number so the same hand replays identically however
  // many times it is asked -- a reload must not deal you a different card.
  Rng rng = worldRng.Derive("jack#" + std::to_string(day) + "#" +
                            std::to_string(handNumber) + "#draw" +
                            std::to_string(hand.draws));
  const int card = DrawCard(rng);
  hand.draws++;
  hand.yours += card;
  if (hand.yours > 21) {
    hand.bust = true;
    hand.finished = true;
  }
  return card;
}

CampfireResult Stand(BlackjackHand& hand, double& cash, double& psyche,
                     std::vector<Partner>& lot, double stake,
                     const Rng& worldRng, int day, int handNumber,
                     const CampfireDials& dials) {
  CampfireResult out;
  // Blackjack is the one with nobody in it, but you are still sitting with
  // them, so the evening still counts for something.
  for (Partner& p : lot) {
    p.rapport = std::min(1.0, p.rapport + dials.rapportPerHand);
  }

  const double put =
      std::max(0.0, std::min(std::min(stake, dials.maxStake),
                             std::max(0.0, cash - dials.ante)));
  const bool wasBust = hand.bust;
  hand.finished = true;

  if (wasBust) {
    out.cashDelta = -(dials.ante + put);
    cash = std::max(0.0, cash + out.cashDelta);
    psyche = std::max(0.05, psyche - dials.psychePerLoss);
    out.line = "Over. Somebody laughs.";
    return out;
  }

  // The dealer plays itself out. Its hole card is drawn here rather than at
  // the deal, on its own key, so peeking at the struct cannot tell you what
  // it has.
  Rng rng = worldRng.Derive("jack#" + std::to_string(day) + "#" +
                            std::to_string(handNumber) + "#house");
  int dealer = hand.dealerShows + DrawCard(rng);
  while (dealer < dials.dealerStandsOn) dealer += DrawCard(rng);

  // A tie pushes. Giving ties to the player is worth several points of
  // edge on its own and was half of why this game printed money.
  if (dealer <= 21 && hand.yours == dealer) {
    out.cashDelta = -dials.ante;
    cash = std::max(0.0, cash - dials.ante);
    out.line = "Split. Nobody moves.";
    return out;
  }

  out.won = dealer > 21 || hand.yours > dealer;
  if (out.won) {
    out.cashDelta = put * dials.blackjackPays;
    cash += out.cashDelta;
    psyche = std::min(1.0, psyche + dials.psychePerWin);
    out.line = dealer > 21 ? "The deck goes over. You take it."
                           : "Yours by " + std::to_string(hand.yours - dealer) +
                                 ".";
  } else {
    out.cashDelta = -(dials.ante + put);
    cash = std::max(0.0, cash + out.cashDelta);
    psyche = std::max(0.05, psyche - dials.psychePerLoss);
    out.line = "The house had " + std::to_string(dealer) + ".";
  }
  return out;
}

FiresideGame WhatsOutTonight(int day) {
  // Days are never negative in play, but a migrated save could hand one
  // over and C++ says -1 % 3 is -1, which would index off the end of every
  // switch below it.
  const int d = ((day % kFiresideGameCount) + kFiresideGameCount) %
                kFiresideGameCount;
  return static_cast<FiresideGame>(d);
}

const char* GameName(FiresideGame game) {
  switch (game) {
    case FiresideGame::Cards:
      return "cards";
    case FiresideGame::Dice:
      return "liar's dice";
    default:
      return "blackjack";
  }
}

double StakeNotch(int notch, const CampfireDials& dials) {
  switch (notch) {
    case 0:
      // The smallest thing you can put in beyond the beer.
      return dials.ante;
    case 1:
      return dials.maxStake * 0.5;
    default:
      // The top notch is the ceiling itself rather than something near it,
      // so a player who has chosen to bet the maximum actually has.
      return dials.maxStake;
  }
}

std::string ReadText(double read) {
  // Deliberately vague at the edges and vaguer in the middle. A number
  // would make this arithmetic; a sentence keeps it a person.
  if (read < 0.15) return "has nothing and knows it";
  if (read < 0.35) return "is not even looking at their cards";
  if (read < 0.5)  return "keeps checking them again";
  if (read < 0.65) return "has gone quiet";
  if (read < 0.85) return "is enjoying this";
  return "has not stopped smiling";
}

}  // namespace dirtbag

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

CampfireHand DealHand(const std::vector<Partner>& lot, const Rng& worldRng,
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

CampfireResult PlayHand(const CampfireHand& hand, double& cash,
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
  for (std::size_t i = 0; i < hand.truth.size(); i++) {
    if (hand.truth[i] < dials.theyStayAbove) continue;
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

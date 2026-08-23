#include "DirtbagTrad.h"

#include <algorithm>
#include <cmath>

#include "DirtbagSport.h"

namespace dirtbag {

bool IsTrad(const Route& route) {
  return route.discipline == Discipline::Trad;
}

double TakesGear(const Move& move, const TradDials& dials) {
  double base = 0.0;
  switch (move.hold) {
    case HoldType::Crack:  base = dials.gearInACrack;  break;
    case HoldType::Jug:    base = dials.gearOnAJug;    break;
    case HoldType::Pocket: base = dials.gearOnAPocket; break;
    case HoldType::Pinch:  base = dials.gearOnAPinch;  break;
    case HoldType::Crimp:  base = dials.gearOnACrimp;  break;
    case HoldType::Sloper: base = dials.gearOnASloper; break;
    case HoldType::Dyno:   base = dials.gearOnADyno;   break;
  }
  // A ledge is a ledge because something structural is happening. This is
  // not the same term as stanceHelpsPlacing below — that one is about
  // whether you can hang around long enough to do a good job, and this one
  // is about whether there is anything there to do it with.
  return Clamp01(base + dials.gearFromAStance * Clamp01(move.restQuality));
}

// How long you have to spend on it, 0..1.
//
// A stance buys you time — but so does the placement being obvious. A
// hand-sized cam into a hand-sized crack goes in off a jam as fast as a
// fiddly RP goes in off a ledge, and the first measured pass of this file
// got that wrong: because a crack move's restQuality is zero unless the
// hold happens to be a jug, *every* placement on a crack route was priced
// as if it were made hanging off a crimp. The bot placed 2.2 pieces on an
// eighteen-move pitch, none of them better than "it might hold", and the
// send rate for leading the thing was zero.
//
// Shared by the quality and the cost, because they are the same question
// asked twice: how much of a fight was that.
static double PlacingEase(const Move& stance, const TradDials& dials) {
  return std::max(Clamp01(stance.restQuality), TakesGear(stance, dials));
}

double PlaceCost(const Move& stance, const TradDials& dials) {
  const double ease = PlacingEase(stance, dials);
  const double bad = 1.0 + (dials.placeFromBadStance - 1.0) * (1.0 - ease);
  return dials.placePumpCost * bad;
}

double PlaceHere(const Move& stance, double pump, const Climber& climber,
                 const Rack& rack, const TradDials& dials) {
  if (rack.pieces <= 0) return 0.0;

  // The rock's ceiling. Nothing below can raise it, which is why a blank
  // wall is a blank wall however good you are.
  double q = TakesGear(stance, dials);
  if (q <= 0.0) return 0.0;

  // Whether the right size was on your harness.
  q *= dials.rackFloor + (1.0 - dials.rackFloor) * Clamp01(rack.quality);

  // Whether you could stand there long enough to do it properly.
  q *= 1.0 - dials.stanceHelpsPlacing * (1.0 - PlacingEase(stance, dials));

  // Whether you were rushing. Gate 2 lives in this line: the same
  // placement, at the same stance, is worse late in a pitch than early.
  q *= 1.0 - dials.pumpSpoilsPlacing * Clamp01(pump / 100.0);

  // Whether you knew what to put where. Technique, because placing gear is
  // pattern recognition and rope work — a strong climber with no idea puts
  // a cam in a flare and a weak one with twenty years puts a nut behind
  // the constriction six inches left.
  const double craft = Clamp01(0.5 + (climber.skills.technique - 50.0) / 100.0);
  q *= 1.0 - dials.craftAtPlacing * (1.0 - craft);

  // And the piece that is worse than nothing on the rope, because it is
  // nothing on the rope and it cost you a piece and the pump to find out.
  if (q < dials.placementFails) return 0.0;
  return Clamp01(q);
}

bool WorthPlacing(const Route& route, int moveIndex, const Protection& gear,
                  const Rack& left, double pump, const Climber& climber,
                  const TradDials& dials) {
  if (!IsTrad(route)) return false;
  if (left.pieces <= 0) return false;

  const int moves = static_cast<int>(route.moves.size());
  if (moveIndex < 0 || moveIndex >= moves) return false;

  // Something is already here. Two pieces at one move is a belay, not a
  // lead.
  if (moveIndex < static_cast<int>(gear.quality.size()) &&
      gear.quality[moveIndex] > 0.0) {
    return false;
  }

  const double likely =
      PlaceHere(route.moves[moveIndex], pump, climber, left, dials);
  if (likely <= 0.0) return false;

  // How much rope is left in the rack against how much pitch is left. A
  // leader who does not do this arithmetic arrives at the headwall with an
  // empty harness, and that is a thing that happens rather than something
  // the game should do to you by default.
  //
  // It raises the *bar* rather than stretching the lookahead. Stretching
  // it was the first attempt and it inverted: with two pieces on a
  // fourteen-move pitch the lookahead ran past the distance fear saturates
  // at, every comparison came out "as bad as it gets either way", and the
  // leader carried both pieces to move ten before placing them next to
  // each other. Being short does not make you see further, it makes you
  // fussier.
  const double movesLeft = static_cast<double>(moves - moveIndex);
  const double wanted =
      movesLeft / std::max(1.0, dials.botSpacing) * dials.botRations;
  const double shortage =
      std::max(1.0, wanted / std::max(1.0, static_cast<double>(left.pieces)));

  // And now the only question that matters: is the stretch of climbing this
  // piece is being bought to cover less frightening with it than without?
  //
  // Both sides are the runout model's own arithmetic — the distance term,
  // the doubt term and the solo term out of DirtbagSport.h — rather than a
  // second opinion about fear. There is one fear model in this project and
  // this is it being *consulted*, which is the same reason the resolver
  // reads RunoutAt instead of guessing.
  const SportDials sport;
  const double sat = std::max(1.0, sport.runoutSaturationMoves);

  // Look ahead one spacing: that is the stretch of climbing this piece is
  // being bought to cover. Never further than the distance fear saturates
  // at, because beyond six moves you are not more frightened and a
  // lookahead past it compares "as bad as it gets" with itself.
  const int reach =
      static_cast<int>(std::lround(std::min(dials.botSpacing, sat)));
  const int ahead = std::min(moves - 1, moveIndex + std::max(1, reach));

  const int last = LastPieceAtOrBelow(route, moveIndex, sport, gear);
  const double lastQ = last >= 0 ? Clamp01(gear.quality[last]) : 0.0;

  // Averaged across the stretch rather than read off its far end, because
  // what a piece buys is *the climbing between here and the next one*. Read
  // at the end alone, a piece that leaves you fully runout by the top of
  // the stretch looks worth exactly nothing, however much of the stretch it
  // made safe.
  //
  // And in grade units on both sides, through the two different dials —
  // because a runout and a ground fall are not the same unit and comparing
  // them as though they were is how a leader talks themselves into
  // soloing.
  const auto fearOver = [&](int from, double quality, bool grounded) {
    double total = 0.0;
    int n = 0;
    for (int j = moveIndex; j <= ahead; j++) {
      total += grounded
                   ? sport.soloGradePenalty * Clamp01(static_cast<double>(j) / sat)
                   : FallPenalty(quality, sport.soloGradePenalty,
                                 sport.runoutGradePenalty,
                                 sport.gearDoubtCurve) *
                         Clamp01(static_cast<double>(j - from) / sat +
                                 (1.0 - quality) * sport.poorGearFear);
      n++;
    }
    return n > 0 ? total / static_cast<double>(n) : 0.0;
  };

  const double withIt = fearOver(moveIndex, likely, false);
  const double without = fearOver(last, lastQ, last < 0);

  // What stopping here costs, in the same grade units the gain is in: the
  // pump you will be carrying for the rest of the pitch. This is the whole
  // decision in one line, and it is why a leader places from the jam and
  // climbs past the crimp — the same gain does not buy the same piece.
  const double cost = PlaceCost(route.moves[moveIndex], dials) / 100.0 *
                      dials.gradesAtFullPump;

  return without - withIt >= (cost + dials.botWorthIt) * shortage;
}

// --- The rack ----------------------------------------------------------------

Rack RackOf(RackTier tier, const TradDials& dials) {
  Rack rack;
  switch (tier) {
    case RackTier::None: break;
    case RackTier::Nuts:
      rack.pieces = dials.nutsPieces;
      rack.quality = dials.nutsQuality;
      break;
    case RackTier::Cams:
      rack.pieces = dials.camsPieces;
      rack.quality = dials.camsQuality;
      break;
    case RackTier::Doubles:
      rack.pieces = dials.doublesPieces;
      rack.quality = dials.doublesQuality;
      break;
  }
  return rack;
}

RackTier TierOf(const Rack& rack, const TradDials& dials) {
  // Read off quality rather than count, because the count is what is left
  // on your harness partway up a pitch and the quality is what you own.
  if (rack.pieces <= 0) return RackTier::None;
  if (rack.quality >= dials.doublesQuality) return RackTier::Doubles;
  if (rack.quality >= dials.camsQuality) return RackTier::Cams;
  if (rack.quality >= dials.nutsQuality) return RackTier::Nuts;
  return RackTier::None;
}

double RackPrice(RackTier tier, const TradDials& dials) {
  switch (tier) {
    case RackTier::None:    return 0.0;
    case RackTier::Nuts:    return dials.nutsCost;
    case RackTier::Cams:    return dials.camsCost;
    case RackTier::Doubles: return dials.doublesCost;
  }
  return 0.0;
}

const char* RackTierName(RackTier tier) {
  switch (tier) {
    case RackTier::None:    return "no rack";
    case RackTier::Nuts:    return "a set of nuts";
    case RackTier::Cams:    return "a rack of cams";
    case RackTier::Doubles: return "doubles";
  }
  return "no rack";
}

bool BuyRack(Rack& rack, double& cash, const TradDials& dials,
             double priceMult) {
  const RackTier have = TierOf(rack, dials);
  if (have == RackTier::Doubles) return false;   // nothing above it
  const RackTier want = static_cast<RackTier>(static_cast<int>(have) + 1);

  const double price = RackPrice(want, dials) * priceMult;
  if (cash < price) return false;

  cash -= price;
  rack = RackOf(want, dials);
  return true;
}

bool CanLeadTrad(const Rack& rack) { return rack.pieces > 0; }

// --- Words -------------------------------------------------------------------

const char* PieceText(double quality) {
  if (quality <= 0.0) return "nothing";
  if (quality < 0.30) return "psychological";
  if (quality < 0.55) return "it might hold";
  if (quality < 0.80) return "that'll do";
  return "bomber";
}

std::string RackText(const Rack& rack, const TradDials& dials) {
  const RackTier tier = TierOf(rack, dials);
  if (tier == RackTier::None) return "no rack";
  std::string out = RackTierName(tier);
  out += ", ";
  out += std::to_string(rack.pieces);
  out += rack.pieces == 1 ? " piece left" : " pieces left";
  return out;
}

}  // namespace dirtbag

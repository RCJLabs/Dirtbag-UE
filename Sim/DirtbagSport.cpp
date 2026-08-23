#include "DirtbagSport.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

std::vector<int> BoltsFor(const Route& route, const SportDials& dials) {
  std::vector<int> bolts;
  if (route.discipline != Discipline::Sport) return bolts;   // boulders: none

  const int moves = static_cast<int>(route.moves.size());
  const double spacing = std::max(1.0, dials.movesPerBolt);
  for (int at = dials.firstBoltAtMove; at < moves;
       at = static_cast<int>(at + spacing)) {
    bolts.push_back(at);
  }
  return bolts;
}

double PieceAt(const Route& route, int moveIndex, const SportDials& dials,
               const Protection& gear) {
  if (moveIndex < 0) return 0.0;
  switch (route.discipline) {
    case Discipline::Boulder:
      return 0.0;
    case Discipline::Sport:
      // Every bolt is a bolt. Nobody has ever backed off a route because
      // they did not fancy the hardware.
      return IsClippingMove(route, moveIndex, dials) ? 1.0 : 0.0;
    case Discipline::Trad:
      if (moveIndex >= static_cast<int>(gear.quality.size())) return 0.0;
      return Clamp01(gear.quality[moveIndex]);
  }
  return 0.0;
}

int LastPieceAtOrBelow(const Route& route, int moveIndex,
                       const SportDials& dials, const Protection& gear) {
  if (route.discipline == Discipline::Sport) {
    // Walk the bolt list once rather than asking PieceAt per move: the
    // answer is the same and the allocation is not quadratic.
    int last = -1;
    for (int bolt : BoltsFor(route, dials)) {
      if (bolt <= moveIndex) last = bolt; else break;
    }
    return last;
  }
  const int top = std::min(moveIndex,
                           static_cast<int>(gear.quality.size()) - 1);
  for (int i = top; i >= 0; i--) {
    if (PieceAt(route, i, dials, gear) > 0.0) return i;
  }
  return -1;
}

bool IsClippingMove(const Route& route, int moveIndex,
                    const SportDials& dials) {
  for (int bolt : BoltsFor(route, dials)) {
    if (bolt == moveIndex) return true;
  }
  return false;
}

double FallPenalty(double trust, double solo, double runout, double curve) {
  const double doubt = 1.0 - Clamp01(trust);
  return runout + (solo - runout) * std::pow(doubt, std::max(1.0, curve));
}

bool OnTheRope(const Route& route, int moveIndex, const SportDials& dials,
               const Protection& gear) {
  if (route.discipline == Discipline::Boulder) return false;
  return LastPieceAtOrBelow(route, moveIndex, dials, gear) >= 0;
}

double RunoutAt(const Route& route, int moveIndex, const SportDials& dials,
                const Protection& gear) {
  if (route.discipline == Discipline::Boulder) return 0.0;

  const int last = LastPieceAtOrBelow(route, moveIndex, dials, gear);
  if (last < 0) {
    // Below the first piece you are not runout, you are bouldering — and
    // the ground-fall penalty that applies there is the right one, which is
    // why this returns nothing rather than everything.
    return 0.0;
  }
  const double above = static_cast<double>(moveIndex - last);
  const double distance =
      Clamp01(above / std::max(1.0, dials.runoutSaturationMoves));

  // And how little you believe in the thing you are above. Zero on sport,
  // always, because PieceAt returns 1 for a bolt — which is what lets one
  // function serve both disciplines without either of them noticing the
  // other exists.
  //
  // They add rather than compete: being a long way above a bad nut is
  // worse than either, and a climber who has been there will tell you so.
  const double doubt =
      (1.0 - PieceAt(route, last, dials, gear)) * dials.poorGearFear;
  return Clamp01(distance + doubt);
}

double ClipCost(const Route& route, int moveIndex, const SportDials& dials) {
  if (!IsClippingMove(route, moveIndex, dials)) return 0.0;
  if (moveIndex < 0 || moveIndex >= static_cast<int>(route.moves.size())) {
    return 0.0;
  }
  // From a jug it is nearly free; from a crimp with your feet cutting it is
  // where routes get lost. The route already describes the stance.
  const double stance = Clamp01(route.moves[moveIndex].restQuality);
  const double bad = 1.0 + (dials.clipPumpFromBadStance - 1.0) * (1.0 - stance);
  return dials.clipPumpCost * bad;
}

bool NeedsABelayer(const Route& route) {
  return route.discipline != Discipline::Boulder;
}

bool WillBelay(const Partner& partner, const SportDials& dials) {
  // The neighbours are not all climbers, and somebody who does not climb is
  // not going to catch a whipper for you.
  if (!partner.climbs) return false;
  return partner.rapport >= dials.minRapportToBelay;
}

int BurnsTheyWillHold(const Partner& partner, const SportDials& dials) {
  if (!WillBelay(partner, dials)) return 0;
  const double t = Clamp01(partner.rapport);
  return static_cast<int>(dials.burnsFromAStranger +
                          (dials.burnsAtFullRapport -
                           dials.burnsFromAStranger) * t);
}

const Partner* BestBelayer(const std::vector<Partner>& lot,
                           const SportDials& dials) {
  const Partner* best = nullptr;
  for (const Partner& p : lot) {
    if (!WillBelay(p, dials)) continue;
    if (!best || p.rapport > best->rapport) best = &p;
  }
  return best;
}

std::string BelayText(const Partner* belayer, const SportDials& dials) {
  if (!belayer) return "nobody is going up there with you today";
  const int burns = BurnsTheyWillHold(*belayer, dials);
  if (burns >= dials.burnsAtFullRapport - 2) {
    return belayer->name + " will hold your rope all afternoon";
  }
  if (burns <= dials.burnsFromAStranger + 1) {
    return belayer->name + " will give you a couple of laps";
  }
  return belayer->name + " is good for a few burns";
}

std::string RunoutText(double runout) {
  if (runout <= 0.0) return "clipped";
  if (runout < 0.34) return "just above the bolt";
  if (runout < 0.67) return "the bolt is below your feet";
  return "a long way above the last clip";
}

}  // namespace dirtbag

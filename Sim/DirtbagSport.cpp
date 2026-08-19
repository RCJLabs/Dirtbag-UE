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

int LastBoltAtOrBelow(const Route& route, int moveIndex,
                      const SportDials& dials) {
  int last = -1;
  for (int bolt : BoltsFor(route, dials)) {
    if (bolt <= moveIndex) last = bolt; else break;
  }
  return last;
}

bool IsClippingMove(const Route& route, int moveIndex,
                    const SportDials& dials) {
  for (int bolt : BoltsFor(route, dials)) {
    if (bolt == moveIndex) return true;
  }
  return false;
}

bool OnTheRope(const Route& route, int moveIndex, const SportDials& dials) {
  if (route.discipline != Discipline::Sport) return false;
  return LastBoltAtOrBelow(route, moveIndex, dials) >= 0;
}

double RunoutAt(const Route& route, int moveIndex, const SportDials& dials) {
  if (route.discipline != Discipline::Sport) return 0.0;

  const int last = LastBoltAtOrBelow(route, moveIndex, dials);
  if (last < 0) {
    // Below the first bolt you are not runout, you are bouldering — and the
    // ground-fall penalty that applies there is the right one, which is why
    // this returns nothing rather than everything.
    return 0.0;
  }
  const double above = static_cast<double>(moveIndex - last);
  return Clamp01(above / std::max(1.0, dials.runoutSaturationMoves));
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

std::string RunoutText(double runout) {
  if (runout <= 0.0) return "clipped";
  if (runout < 0.34) return "just above the bolt";
  if (runout < 0.67) return "the bolt is below your feet";
  return "a long way above the last clip";
}

}  // namespace dirtbag

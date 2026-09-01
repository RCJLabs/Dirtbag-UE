#include "DirtbagMonotony.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

double Clamped(double v, double lo, double hi) {
  return std::min(hi, std::max(lo, v));
}

}  // namespace

Stepped Feed(Monotony& monotony, Skill lane, const std::string& stimulus,
             const MonotonyDials& dials) {
  const int i = static_cast<int>(lane);
  const double was = monotony.level[i];
  const bool same = monotony.lastStimulus[i] == stimulus;

  monotony.level[i] = same ? std::min(1.0, was + dials.rise)
                           : std::max(0.0, was - dials.novelDrop);
  monotony.lastStimulus[i] = stimulus;

  Stepped out;
  // **Measured against what it was, not what it is now.** Reading the shed
  // level would mean a plateau can never be broken: the drop happens first,
  // so by the time you ask, the thing you broke is gone.
  out.breakthrough = !same && was >= dials.breakthroughAt;
  out.gainMultiplier = (1.0 - monotony.level[i] * dials.penalty) *
                       (out.breakthrough ? 1.0 + dials.breakthroughBonus : 1.0);
  return out;
}

void SleptOn(Monotony& monotony, bool deloading, const MonotonyDials& dials) {
  const double shed = dials.restShed + (deloading ? dials.deloadShed : 0.0);
  for (int i = 0; i < kSkillCount; i++) {
    monotony.level[i] = std::max(0.0, monotony.level[i] - shed);
  }
}

std::string PlateauLine(const Monotony& monotony, const MonotonyDials& dials) {
  int worst = -1;
  for (int i = 0; i < kSkillCount; i++) {
    if (monotony.level[i] < dials.breakthroughAt) continue;
    if (worst < 0 || monotony.level[i] > monotony.level[worst]) worst = i;
  }
  if (worst < 0) return std::string();
  return std::string("Your ") + SkillName(static_cast<Skill>(worst)) +
         " has stopped answering. It has seen this before.";
}

void ClimbedAt(VenueStaleness& stale, const std::string& venue,
               const MonotonyDials& dials) {
  if (stale.venue == venue) {
    stale.days = std::min(dials.staleMax, stale.days + dials.staleRise);
    return;
  }
  // Somewhere else. The counter falls rather than resetting, because two
  // days at a new crag does not undo a season in the same gym -- and it
  // falls three times as fast as it rose, which is what keeps one good trip
  // worth taking.
  stale.days = std::max(0.0, stale.days - dials.staleFall);
  stale.venue = venue;
}

double Freshness(const VenueStaleness& stale, const MonotonyDials& dials) {
  const double left =
      Clamped(1.0 - stale.days / std::max(1e-9, dials.staleMax), 0.0, 1.0);
  return dials.staleFloor +
         (1.0 - dials.staleFloor) * std::pow(left, dials.stalePower);
}

std::string StaleLine(const VenueStaleness& stale,
                      const MonotonyDials& dials) {
  // Two thirds of the way to flat, which is where it starts being a thing
  // you would say out loud rather than a number going down.
  if (stale.days < dials.staleMax * 0.66 || stale.venue.empty()) {
    return std::string();
  }
  if (stale.days >= dials.staleMax) {
    return "You could climb here with your eyes shut, and lately you have "
           "been.";
  }
  return "The same walls, again. It is starting to show.";
}

}  // namespace dirtbag

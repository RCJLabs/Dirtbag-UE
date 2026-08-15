#include "DirtbagSession.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

namespace {

// Per-move odds sit on a sigmoid around this bias: at skill parity, a fresh
// climber lands the move sigmoid(kOddsBias) ≈ 82% of the time. The rest of
// the curve is the dial's business.
constexpr double kOddsBias = 1.5;

double Sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

// Which skills a hold type actually asks for. Weights sum to 1.
double BlendedSkill(const Skills& s, HoldType hold) {
  switch (hold) {
    case HoldType::Crimp:  return s.fingers * 0.7 + s.technique * 0.3;
    case HoldType::Sloper: return s.power * 0.5 + s.technique * 0.3 + s.fingers * 0.2;
    case HoldType::Pinch:  return s.power * 0.4 + s.fingers * 0.4 + s.endurance * 0.2;
    case HoldType::Pocket: return s.fingers * 0.6 + s.technique * 0.4;
    case HoldType::Jug:    return s.endurance * 0.5 + s.technique * 0.3 + s.power * 0.2;
    case HoldType::Dyno:   return s.power * 0.7 + s.head * 0.3;
    case HoldType::Crack:  return s.technique * 0.6 + s.endurance * 0.3 + s.head * 0.1;
  }
  return s.technique;
}

// Reach fit in grade units: Lanky loves reachy moves and hates scrunchy ones,
// Compact the reverse, Powerful gets its bonus on burl instead of span.
double MorphologyAdjust(const Climber& climber, const Move& move,
                        const SessionDials& dials) {
  double fit = 0.0;
  switch (climber.morphology) {
    case Morphology::Lanky:   fit = move.reachBias; break;
    case Morphology::Compact: fit = -move.reachBias; break;
    case Morphology::Average: fit = 0.0; break;
    case Morphology::Powerful:
      fit = (move.hold == HoldType::Dyno || move.hold == HoldType::Sloper) ? 0.6 : 0.0;
      break;
  }
  return fit * dials.morphologyWeight * 0.1;  // full fit ≈ 0.4 grades
}

}  // namespace

AttemptResult ResolveAttempt(Rng& rng, const AttemptInput& input,
                             const SessionDials& dials) {
  AttemptResult result;
  const Climber& c = input.climber;
  const Route& r = input.route;

  double pump = 0.0;

  for (size_t i = 0; i < r.moves.size(); i++) {
    const Move& move = r.moves[i];
    const double exec = i < input.execution.size()
                            ? std::clamp(input.execution[i], 0.0, 1.0)
                            : input.botExecution;

    // Effective ability on this move, in grade units (skill 0..100 spans the
    // V0..V18 ladder, exactly as the 2D game's skill-vs-grade check does).
    double effective = BlendedSkill(c.skills, move.hold) / 100.0 * kMaxGrade;

    // The minigame's say: a perfect move beats a botched one by ~25 skill
    // points (executionWeight 0.5 → ±2.25 grades either side of neutral).
    effective += (exec - 0.5) * dials.executionWeight * 9.0;

    // Conditions, beta, morphology, skin, nerve.
    effective += (input.conditions.friction - 0.5) * dials.frictionWeight * 0.1;
    effective += input.beta * 0.5;
    effective += MorphologyAdjust(c, move, dials);
    // Body and head state: both default to neutral (warm, ordinary-day
    // psyche), so only session-loop callers feel them.
    effective -= dials.coldStartPenalty * (1.0 - std::clamp(input.warmth, 0.0, 1.0));
    effective += (c.psyche - 0.7) * dials.psycheWeight;
    const bool skinHold = move.hold == HoldType::Crimp || move.hold == HoldType::Pocket;
    if (skinHold && c.skin < 3.0) {
      effective -= dials.thinSkinPenalty * (3.0 - c.skin);
    }
    if (move.crux) {
      // The crux is where the head shows up — commitment, not strength.
      effective += (c.skills.head - 50.0) / 100.0;
    }

    // Pump spends ability, in grade units — felt only where margins are thin.
    effective -= dials.pumpGradePenalty * (pump / 100.0);

    const double margin = effective - move.difficulty;
    const double odds = Sigmoid(kOddsBias + margin * dials.oddsSlope);

    MoveResult mr;
    mr.index = static_cast<int>(i);
    mr.odds = odds;

    // Redline: fully pumped hands open regardless of the move.
    const bool redlined = pump >= 100.0;
    mr.success = !redlined && rng.Chance(odds);

    // Pump accounting: harder-than-you moves cost more; endurance and clean
    // execution (no over-gripping) both pay it down; rests pay it back.
    double cost = dials.basePumpCost +
                  dials.pumpPerDifficulty * std::max(0.0, move.difficulty - effective);
    cost *= 1.0 - dials.enduranceRelief * (c.skills.endurance / 100.0);
    cost *= 1.3 - 0.6 * exec;
    pump = std::min(100.0, pump + std::max(2.0, cost));
    if (mr.success && move.restQuality > 0.0) {
      pump = std::max(0.0, pump - move.restQuality * dials.restRecovery);
    }

    mr.pumpAfter = pump;
    result.peakPump = std::max(result.peakPump, pump);
    result.timeline.push_back(mr);

    if (!mr.success) break;
    result.highpoint = static_cast<int>(i) + 1;
  }

  result.sent = result.highpoint == static_cast<int>(r.moves.size());
  if (result.sent) {
    if (input.attemptNumber == 1) {
      result.style = input.beta < 0.05 ? Style::Onsight : Style::Flash;
    } else {
      result.style = Style::Redpoint;
    }
  } else {
    result.style = Style::Fell;
  }

  // Skin: the 2D rule — falls cost a point; sends wear less; crimpy mileage
  // adds its tax either way.
  int crimpMoves = 0;
  for (const MoveResult& mr : result.timeline) {
    const HoldType h = r.moves[mr.index].hold;
    if (h == HoldType::Crimp || h == HoldType::Pocket) crimpMoves++;
  }
  result.skinCost = (result.sent ? dials.sendSkinCost : dials.fallSkinCost) +
                    0.05 * crimpMoves;

  return result;
}

}  // namespace dirtbag

#include "DirtbagSession.h"

#include "DirtbagBody.h"
#include "DirtbagGear.h"
#include "DirtbagSport.h"

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

// Effective ability on a move, in grade units (skill 0..100 spans the
// V0..V18 ladder, exactly as the 2D game's skill-vs-grade check does).
// Shared by the live step and the odds preview so the UI never lies.
double MoveEffective(const AttemptInput& input, const Move& move, int index,
                     double exec, double pump, const SessionDials& dials) {
  const Climber& c = input.climber;
  double effective = SkillToGrade(BlendedSkill(c.skills, move.hold), dials);

  // The minigame's say: a perfect move beats a botched one by ~25 skill
  // points (executionWeight 0.5 → ±2.25 grades either side of neutral).
  effective += (exec - 0.5) * dials.executionWeight * 9.0;

  // Conditions, beta, morphology, skin, nerve.
  effective += (input.conditions.friction - 0.5) * dials.frictionWeight * 0.1;

  // Dirt reads as ability you do not have: no chalk sticks, the feet are
  // gravel, and the holds are somewhere under the moss.
  effective -= dials.dirtGradePenalty * (1.0 - input.cleanliness);
  // Knowing the sequence, worth more the more sequence there is.
  {
    const int n = static_cast<int>(input.route.moves.size());
    const double t =
        Clamp01(static_cast<double>(n - dials.betaFlatUntilMoves) /
                std::max(1.0, static_cast<double>(dials.betaLongAtMoves -
                                                  dials.betaFlatUntilMoves)));
    const double worth =
        dials.betaGradeValue * (1.0 + (dials.betaValueAtLength - 1.0) * t);
    effective += input.beta * worth;
  }
  effective += MorphologyAdjust(c, move, dials);
  // Body and head state: both default to neutral (warm, ordinary-day
  // psyche), so only session-loop callers feel them.
  effective -= dials.coldStartPenalty * (1.0 - std::clamp(input.warmth, 0.0, 1.0));
  effective += (c.psyche - 0.7) * dials.psycheWeight;
  const bool skinHold = move.hold == HoldType::Crimp || move.hold == HoldType::Pocket;
  const double worn =
      Clamp01(1.0 - c.skin / std::max(0.001, dials.freshSkin));
  effective -= dials.skinGradePenalty * worn * worn *
               (skinHold ? 1.0 : dials.skinBiteOnGoodHolds);

  // Rubber. Edging holds punish dead shoes hardest, which is why a worn
  // pair pushes you onto slopers and jugs long before it stops you.
  const bool edging = move.hold == HoldType::Crimp ||
                      move.hold == HoldType::Pocket ||
                      move.hold == HoldType::Pinch;
  effective -= input.oddsPenalty;
  effective -= ShoePenaltyFor(input.shoeWear, edging,
                              dials.deadShoeGradePenalty,
                              dials.shoeBiteOnGoodHolds);
  if (move.crux) {
    // The crux is where the head shows up — commitment, not strength.
    effective += (c.skills.head - 50.0) / 100.0;
  }

  // What is currently wrong with you, and only where it bites. A pulley is
  // over on crimps and fine on slopers, which is why an injured climber
  // becomes a sloper climber for a month rather than stopping — the choice
  // of what to get on is the injury's actual gameplay.
  if (c.injury.active) {
    effective -= dials.injuryGradePenalty * c.injury.severity *
                 InjuryBiteOn(c.injury.kind, move.hold);
  }

  // What you would hit, which is a different question on a rope than on a
  // pad. Both are paid out of head — a bold climber is still bolder — but
  // they are not the same fear and they must not both apply.
  // Head is how well you handle being up there; boldness is how much it
  // bothers you. They are different questions and they add.
  const double nerve =
      Clamp01(0.5 + (c.skills.head - 50.0) / 100.0 + input.boldness);
  effective -= ExposureAt(input.route, index, input.padding, dials) *
               (1.0 - 0.5 * nerve);

  // Pump spends ability, in grade units — felt only where margins are thin.
  effective -= dials.pumpGradePenalty * (pump / 100.0);
  return effective;
}

}  // namespace

double ExposureAt(const Route& route, int index, double padding,
                  const SessionDials& dials) {
  // What you would hit, which is a different question on a rope than on a
  // pad. Both are paid out of head — a bold climber is still bolder — but
  // they are not the same fear and they must not both apply.
  if (OnTheRope(route, index)) {
    // Above the first bolt the ground is not the question any more. The
    // crash-pad penalty has to *stop* here or it follows a roped climber
    // thirty metres up a pitch and punishes them for having no foam under
    // a route nobody would put foam under.
    return dials.runoutGradePenalty * RunoutAt(route, index);
  }
  const int moves = static_cast<int>(route.moves.size());
  if (moves <= 0) return 0.0;

  // The landing. Bare ground costs nothing low down — nobody has ever been
  // gripped on move one — and climbs toward the top, which is why a pad is
  // worth its price exactly where a boulderer is trying hardest. This still
  // applies to the first moves of a pitch, below the first bolt, which is
  // exactly where it should.
  const double up = static_cast<double>(index + 1) / static_cast<double>(moves);
  const double exposed = Clamp01((up - dials.padGroundedFraction) /
                                 std::max(0.001, 1.0 - dials.padGroundedFraction));
  return dials.noPadGradePenalty * (1.0 - Clamp01(padding)) * exposed;
}

double SkillToGrade(double skill, const SessionDials& dials) {
  return skill / 100.0 * dials.skillGradeSpan + dials.skillGradeFloor;
}

double AbilityOnRoute(const Climber& climber, const Route& route,
                      const SessionDials& dials) {
  if (route.moves.empty()) return SkillToGrade(0.0, dials);
  double total = 0.0;
  for (const Move& move : route.moves) {
    total += BlendedSkill(climber.skills, move.hold);
  }
  return SkillToGrade(total / static_cast<double>(route.moves.size()), dials);
}

RouteRead ReadRoute(const Climber& climber, const Route& route,
                    const SessionDials& dials) {
  // The guidebook's opinion, not the rock's — you can't see a sandbag.
  const double gap =
      static_cast<double>(route.grade) - AbilityOnRoute(climber, route, dials);
  if (gap <= -2.0) return RouteRead::Warmup;
  if (gap <= -0.5) return RouteRead::Comfortable;
  if (gap <= 1.0) return RouteRead::AtYourLimit;
  if (gap <= 2.5) return RouteRead::Project;
  return RouteRead::NotThisYear;
}

const char* ReadRouteText(RouteRead read) {
  switch (read) {
    case RouteRead::Warmup:      return "Warmup pace. Save something for later.";
    case RouteRead::Comfortable: return "This should go.";
    case RouteRead::AtYourLimit: return "Your grade, on a good day.";
    case RouteRead::Project:     return "A project. Bring skin and patience.";
    case RouteRead::NotThisYear: return "Not this year.";
  }
  return "";
}

LiveAttempt BeginAttempt(const Rng& rng, const AttemptInput& input,
                         const SessionDials& dials) {
  LiveAttempt la;
  la.input = input;
  la.dials = dials;
  la.rng = rng;
  return la;
}

bool AttemptOver(const LiveAttempt& la) {
  return la.over ||
         la.nextMove >= static_cast<int>(la.input.route.moves.size());
}

double PeekOdds(const LiveAttempt& la, double execution) {
  if (AttemptOver(la)) return 0.0;
  const Move& move = la.input.route.moves[la.nextMove];
  const double exec = std::clamp(execution, 0.0, 1.0);
  const double margin =
      MoveEffective(la.input, move, la.nextMove, exec, la.pump, la.dials) -
      move.difficulty;
  return Sigmoid(kOddsBias + margin * la.dials.oddsSlope);
}

MoveResult StepMove(LiveAttempt& la, double execution) {
  if (AttemptOver(la)) return MoveResult{};
  const Move& move = la.input.route.moves[la.nextMove];
  const double exec = std::clamp(execution, 0.0, 1.0);

  const double effective =
      MoveEffective(la.input, move, la.nextMove, exec, la.pump, la.dials);
  const double margin = effective - move.difficulty;
  const double odds = Sigmoid(kOddsBias + margin * la.dials.oddsSlope);

  MoveResult mr;
  mr.index = la.nextMove;
  mr.odds = odds;

  // Redline: fully pumped hands open regardless of the move. No roll is
  // consumed — a redlined fall is not luck.
  const bool redlined = la.pump >= 100.0;
  mr.success = !redlined && la.rng.Chance(odds);

  // Pump accounting: harder-than-you moves cost more; endurance and clean
  // execution (no over-gripping) both pay it down.
  double cost = la.dials.basePumpCost +
                la.dials.pumpPerDifficulty *
                    std::max(0.0, move.difficulty - effective);
  cost *= 1.0 - la.dials.enduranceRelief * (la.input.climber.skills.endurance / 100.0);
  cost *= 1.3 - 0.6 * exec;

  // Pulling up slack with one hand off a hold. Nearly free from a jug, and
  // from a crimp with your feet cutting it is where routes get lost — which
  // is the whole reason a clipping stance is a thing climbers talk about.
  // Endurance and clean execution do not pay this down: the rope weighs
  // what it weighs.
  cost += ClipCost(la.input.route, la.nextMove);

  la.pump = std::min(100.0, la.pump + std::max(2.0, cost));

  mr.pumpAfter = la.pump;
  la.partial.peakPump = std::max(la.partial.peakPump, la.pump);
  la.partial.timeline.push_back(mr);

  if (mr.success) {
    la.nextMove++;
    la.partial.highpoint = la.nextMove;
    la.shakesAtStance = 0;
  } else {
    la.over = true;
  }
  return mr;
}

double ShakeOut(LiveAttempt& la) {
  if (AttemptOver(la) || la.nextMove == 0) return 0.0;
  const Move& stance = la.input.route.moves[la.nextMove - 1];
  const double before = la.pump;

  // First shake is the stance's full value; each repeat halves and pays the
  // hang tax. The timeline's pumpAfter records the stance as it was left,
  // so a staged replay shows the shake, not the arrival.
  double recovery = stance.restQuality * la.dials.restRecovery;
  for (int i = 0; i < la.shakesAtStance; i++) recovery *= la.dials.shakeDiminish;
  const double hang = la.shakesAtStance == 0 ? 0.0 : la.dials.shakeHangCost;
  la.pump = std::clamp(la.pump - recovery + hang, 0.0, 100.0);
  la.shakesAtStance++;

  la.partial.timeline.back().pumpAfter = la.pump;
  return before - la.pump;
}

AttemptResult FinishAttempt(const LiveAttempt& la) {
  const Route& r = la.input.route;
  AttemptResult result = la.partial;

  result.sent = result.highpoint == static_cast<int>(r.moves.size());
  if (result.sent) {
    if (la.input.attemptNumber == 1) {
      result.style = la.input.beta < 0.05 ? Style::Onsight : Style::Flash;
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
  result.skinCost = (result.sent ? la.dials.sendSkinCost : la.dials.fallSkinCost) +
                    0.05 * crimpMoves;

  return result;
}

AttemptResult ResolveAttempt(Rng& rng, const AttemptInput& input,
                             const SessionDials& dials) {
  // The batch form is the live form with a bot at the controls: fixed
  // execution per move, one shake-out at every stance that offers one.
  LiveAttempt la = BeginAttempt(rng, input, dials);
  while (!AttemptOver(la)) {
    const size_t i = static_cast<size_t>(la.nextMove);
    const double exec = i < input.execution.size()
                            ? std::clamp(input.execution[i], 0.0, 1.0)
                            : input.botExecution;
    const MoveResult mr = StepMove(la, exec);
    if (mr.success && input.route.moves[i].restQuality > 0.0) ShakeOut(la);
  }
  rng = la.rng;  // the caller's stream advances exactly as it always did
  return FinishAttempt(la);
}


double HowClose(const AttemptResult& result, int totalMoves,
                const CloseDials& dials) {
  if (result.sent) return 1.0;
  // A route with no moves cannot be got up or fallen off.
  if (totalMoves <= 0) return 0.0;

  const double progress =
      std::min(1.0, static_cast<double>(result.highpoint) /
                        static_cast<double>(totalMoves));
  const double base = std::pow(progress, dials.topHeavy);

  // What you fell off. The last entry in the timeline is the move that
  // ended it -- unless the timeline is empty, which happens when the
  // attempt never started, and then there is nothing to have been close
  // to.
  if (result.timeline.empty()) return 0.0;
  const double failedOdds = result.timeline.back().odds;

  // Falling off a gimme costs nothing; falling off a desperate move
  // discounts the whole attempt by `blownIt` at the limit. Bounded to
  // [0, 1] by construction: base is, and the factor runs 1 - blownIt to 1.
  return base * (1.0 - dials.blownIt + dials.blownIt * failedOdds);
}

double PumpShows(double pump, const ShowDials& dials) {
  const double top = 100.0;
  if (pump <= dials.quietBelow) return 0.0;
  if (dials.quietBelow >= top) return 0.0;
  const double t = (std::min(pump, top) - dials.quietBelow) /
                   (top - dials.quietBelow);
  return std::pow(t, dials.curve);
}

const char* HowCloseText(double close) {
  // Six bands, and none of them a number. The point of the gate is that
  // somebody watching over your shoulder knows what happened, and "you
  // fell at move 9 of 12" is a thing you read rather than a thing you see.
  if (close >= 0.999) return "Done.";
  if (close >= 0.82) return "One move. That was the go.";
  if (close >= 0.60) return "You had it up there.";
  if (close >= 0.35) return "Got into it, and it got you back.";
  if (close >= 0.12) return "A burn.";
  return "Off early. Nothing learned but the first two moves.";
}

}  // namespace dirtbag

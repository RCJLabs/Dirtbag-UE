#include "DirtbagSessionLoop.h"

#include <algorithm>

namespace dirtbag {

SessionState StartSession(const Climber& climber) {
  SessionState s;
  s.skinLeft = climber.skin;
  s.psyche = climber.psyche;
  return s;
}

AttemptResult AttemptInSession(const Rng& sessionRng, SessionState& session,
                               ProjectMemory& memory, const Climber& climber,
                               const Route& route, const Conditions& conditions,
                               const std::vector<double>& execution,
                               double botExecution, const SessionDials& dials,
                               const SessionLoopDials& loop) {
  // Salt by route and lifetime attempt number: burn #7 on a project rolls the
  // same dice whether it happens today or is replayed from a save.
  Rng rng = sessionRng.Derive(route.name + "#" +
                              std::to_string(memory.attempts + 1));

  AttemptInput in;
  in.climber = climber;
  in.climber.skin = session.skinLeft;    // the body as it is now,
  in.climber.psyche = session.psyche;    // not as it woke up
  in.route = route;
  in.conditions = conditions;
  in.beta = memory.beta;
  in.attemptNumber = memory.attempts + 1;
  in.warmth = session.warmth;
  in.execution = execution;
  in.botExecution = botExecution;

  const AttemptResult result = ResolveAttempt(rng, in, dials);

  // The session pays for the burn: skin spent, warmth earned per move
  // actually climbed (a one-move flail warms nobody up).
  session.skinLeft = std::max(0.0, session.skinLeft - result.skinCost);
  session.warmth = std::min(
      1.0, session.warmth + loop.warmupPerMove *
                                static_cast<double>(result.timeline.size()));
  session.attemptsMade++;

  const bool newHighpoint = result.highpoint > memory.bestHighpoint;
  if (result.sent) {
    session.psyche = std::min(1.0, session.psyche + loop.psycheSendGain);
  } else if (newHighpoint) {
    session.psyche = std::min(1.0, session.psyche + loop.psycheHighpointGain);
  } else {
    session.psyche =
        std::max(loop.psycheFloor, session.psyche - loop.psycheFailLoss);
  }

  // The ledger. Beta counts moves touched — the one that spat you off
  // included — as a fraction of the route; new ground converts at
  // betaLearnRate, known ground refines but never past the historical
  // highwater (touched fraction), so lapping the start can't unlock the crux.
  memory.routeName = route.name;
  memory.attempts++;
  memory.bestHighpoint = std::max(memory.bestHighpoint, result.highpoint);
  if (!route.moves.empty()) {
    const double moveCount = static_cast<double>(route.moves.size());
    const double touched =
        static_cast<double>(result.timeline.size()) / moveCount;
    const double touchedBest =
        memory.sent || result.sent
            ? 1.0
            : std::min(1.0, (memory.bestHighpoint + 1) / moveCount);
    if (touched > memory.beta) {
      memory.beta += (touched - memory.beta) * loop.betaLearnRate;
    } else {
      memory.beta = std::min(memory.beta + loop.betaRehearsalGain, touchedBest);
    }
  }
  if (result.sent && !memory.sent) {
    memory.sent = true;
    memory.firstSendStyle = result.style;
  }

  return result;
}

}  // namespace dirtbag

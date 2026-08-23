#include "DirtbagSessionLoop.h"

#include <algorithm>

namespace dirtbag {

SessionState StartSession(const Climber& climber) {
  SessionState s;
  s.skinLeft = climber.skin;
  s.psyche = climber.psyche;
  return s;
}

Rng DeriveAttemptRng(const Rng& sessionRng, const ProjectMemory& memory,
                     const Route& route) {
  // Salt by route and lifetime attempt number: burn #7 on a project rolls the
  // same dice whether it happens today or is replayed from a save.
  return sessionRng.Derive(route.name + "#" +
                           std::to_string(memory.attempts + 1));
}

SessionAdvice ReadSession(const SessionState& session, const Climber& climber,
                          const SessionDials& dials,
                          const SessionLoopDials& loop) {
  (void)climber;
  (void)dials;
  const bool cold = session.warmth < loop.coldBelowWarmth;
  const bool thin = session.skinLeft < loop.thinSkin;

  if (session.skinLeft < loop.spentSkin) return SessionAdvice::Wrecked;
  if (cold && thin) return SessionAdvice::Wrecked;
  if (cold) return SessionAdvice::Cold;
  if (thin) return SessionAdvice::SkinThin;
  return SessionAdvice::Ready;
}

const char* SessionAdviceText(SessionAdvice advice) {
  switch (advice) {
    case SessionAdvice::Ready:    return "warm, and there is skin on your fingers";
    case SessionAdvice::Cold:     return "still cold — pull on something easy first";
    case SessionAdvice::SkinThin: return "skin is going; better holds or better luck";
    case SessionAdvice::Wrecked:  return "that is the day. Come back tomorrow";
  }
  return "";
}

AttemptInput BuildSessionAttemptInput(const SessionState& session,
                                      const ProjectMemory& memory,
                                      const Climber& climber,
                                      const Route& route,
                                      const Conditions& conditions,
                                      const std::vector<double>& execution,
                                      double botExecution,
                                      const Character& who) {
  AttemptInput in;
  in.climber = climber;
  in.climber.skin = session.skinLeft;    // the body as it is now,
  in.climber.psyche = session.psyche;    // not as it woke up
  in.route = route;
  in.conditions = conditions;
  in.beta = memory.beta;
  in.attemptNumber = memory.attempts + 1;
  in.warmth = session.warmth;
  in.cleanliness = memory.cleanliness;   // how much of it you have uncovered
  in.shoeWear = session.shoeWear;        // what is left of the rubber
  in.padding = session.padding;          // what you dragged up the hill
  // Happy Feet, and nothing else in this game, touches this.
  in.oddsPenalty = OddsPenalty(who, route.type);
  in.boldness = NerveShift(who);
  in.execution = execution;
  in.botExecution = botExecution;
  return in;
}

void CommitAttempt(SessionState& session, ProjectMemory& memory,
                   const Route& route, const AttemptResult& result,
                   const SessionLoopDials& loop) {
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
  // The ledger keeps the grade itself: a career has to be able to answer
  // "what do you climb?" long after a gym has reset the wall.
  memory.grade = route.grade;
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
}

AttemptResult AttemptInSession(const Rng& sessionRng, SessionState& session,
                               ProjectMemory& memory, const Climber& climber,
                               const Route& route, const Conditions& conditions,
                               const std::vector<double>& execution,
                               double botExecution, const SessionDials& dials,
                               const SessionLoopDials& loop) {
  Rng rng = DeriveAttemptRng(sessionRng, memory, route);
  const AttemptInput in = BuildSessionAttemptInput(
      session, memory, climber, route, conditions, execution, botExecution);
  const AttemptResult result = ResolveAttempt(rng, in, dials);
  CommitAttempt(session, memory, route, result, loop);
  return result;
}

}  // namespace dirtbag

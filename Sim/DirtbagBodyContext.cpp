#include "DirtbagBodyContext.h"

namespace dirtbag {

void ApplyBody(AttemptInput& in, const BodyContext& body) {
  // Happy Feet, and nothing else about where you came from, is allowed
  // near send odds. **Added rather than assigned**: a comp's pressure is
  // already on this field and the two are different reasons.
  in.oddsPenalty += OddsPenalty(body.who, in.route.type);
  // Temperament: how steady you are above the last piece, over and above
  // what your head skill says.
  in.boldness += NerveShift(body.who);
  // And what you have become since, which is the same axis arriving from
  // the other direction: a temperament is what you were like and a habit is
  // what you have been doing. They add, for the same reason a comp's nerves
  // add to a flaw -- two reasons, not one replacing the other.
  in.boldness += HabitNerve(body.quirks, body.logbook, body.day);
  // And the third reason, from outside climbing entirely: what a phone
  // call home does is remind you there is a version of you that is not
  // this. Adds, like the other two.
  in.boldness += LifeNerve(body.life);

  // What is left of the rubber. Assigned rather than added, because it is
  // a state and not a modifier.
  in.shoeWear = body.shoeWear;

  // Being ill, and the tooth. Flat on every hold, because neither is about
  // what you are holding on to.
  in.ailmentPenalty += SickPenalty(body.sickness) +
                       ToothGradePenalty(body.teeth);

  // What the joints carry, which does not heal.
  for (int j = 0; j < kInjuryKindCount; j++) {
    in.jointDamage[j] =
        JointWear(body.medical, static_cast<InjuryKind>(j), body.day);
  }

  // How far through the comeback you are. A graded return is climbing --
  // badly -- and this is the number that says so.
  if (in.climber.injury.active && body.medical.stage != Comeback::Clear) {
    in.injuryStagePenalty = StagePenalty(body.medical);
  }
}

}  // namespace dirtbag

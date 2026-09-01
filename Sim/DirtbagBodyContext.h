#pragma once

// Everything about the body that follows a climber into any attempt,
// anywhere.
//
// **This file exists because of a measured hole.** `AttemptProblem` — the
// comp resolver — built its own `AttemptInput` from scratch and set six
// fields. It never set the joints, the ailments, the comeback stage, the
// flaw or the temperament, so a climber with a maxed-out finger joint, four
// cortisone shots, a bad flu and an abscess **scored identically to a
// healthy one at a comp**:
//
//     comp score, healthy climber:      4.1
//     comp score, same climber wrecked: 4.1
//
// Through the session path the same body carried 2.2 grades of ailment
// penalty and a fully degraded joint. Two paths, one of them assembling the
// input by hand — the same shape as `Injury::staged`'s two owners, and the
// same shape as every "written and never wired" this project has found.
//
// That path serves gym comps, the circuit, the World Cup, the Games and
// league nights, which is most of the game's indoor time. Two whole phases
// were invisible there.
//
// ## What is here and what is not
//
// **Here: the body.** Who this climber is, what their joints carry, what
// they have caught, what their teeth are doing, what is left of their
// rubber. All of it follows them everywhere and none of it cares where
// they are standing.
//
// **Not here: the situation.** Warmth, mats, cleanliness, beta, and the
// nerves of a competition are exactly what make a comp different from a
// Tuesday, and they belong to the caller. `ApplyBody` never touches them.
//
// Engine-free like everything in Sim/.

#include "DirtbagAilments.h"
#include "DirtbagCharacter.h"
#include "DirtbagHabits.h"
#include "DirtbagLife.h"
#include "DirtbagMedical.h"
#include "DirtbagSession.h"
#include "DirtbagStyle.h"

namespace dirtbag {

struct BodyContext {
  // Who they are. Only the flaw and the temperament reach an attempt --
  // see Sim/DirtbagCharacter.h, where origins are deliberately kept away
  // from send odds.
  Character who;
  // What the joints carry, and how far through a comeback they are.
  Medical medical;
  // What they have caught, and what their teeth are doing.
  Sickness sickness;
  Teeth teeth;
  // What is left of the rubber. **Zero is new shoes**, and a comp on
  // permanently new rubber was the quietest part of the same hole.
  double shoeWear = 0.0;
  // **What you have been doing lately, and what it made you.** Here rather
  // than on the caller's side because a habit is part of the body you climb
  // in, and because this struct exists precisely so that a second attempt
  // path cannot be built that forgets half of one. See Sim/DirtbagHabits.h.
  Quirks quirks;
  Logbook logbook;
  // **And the life outside it.** Here rather than on the caller's side
  // because a phone call home follows you to a comp exactly as a bad tooth
  // does -- it is not about where you are standing, which is this struct's
  // whole test. Only the steadiness reaches an attempt; psyche is the day
  // loop's business and money is nobody's. See Sim/DirtbagLife.h.
  Life life;
  // **And what a career of climbing made you.** Here for the same reason
  // the tooth is: a specialist is a specialist at a comp, on a comp board
  // somebody else set, exactly as much as at their home crag. Nothing else
  // in the game reads the route type against the climber's own history, so
  // leaving it to the callers would mean the two attempt paths disagreed
  // about who you are. See Sim/DirtbagStyle.h.
  StyleLog style;
  Signature signature;
  Signature signature2;
  // Scars and joints are priced off the date. The logbook is too.
  int day = 0;
};

// Stamp a body onto an attempt. **Call this at every site that builds an
// `AttemptInput`**, after the route is set — `OddsPenalty` reads the route
// type, because a flaw is about what kind of climbing it is.
//
// Additive where the caller has already had its say: the comp's pressure
// and a climber's flaw are two different reasons the odds are worse and
// they add, rather than one silently replacing the other.
void ApplyBody(AttemptInput& in, const BodyContext& body);

}  // namespace dirtbag

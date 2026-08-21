// The first-ascent pipeline: clean → work → send → name.
//
// Phase 2's headline gate is "a new player names their own first ascent",
// and this is the whole arc of that. It is deliberately not a quest: there
// is no lock to pick and nothing grants permission. A virgin line is simply
// filthy, and filthy rock climbs about four grades harder than it will once
// you have spent hours on it with a brush. You may throw yourself at it
// dirty and you will fail, which is the honest version of a gate.
//
// What you get for finishing is the thing no other route can give you: the
// grade nobody knew, and the naming.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagCrag.h"
#include "DirtbagDay.h"
#include "DirtbagSessionLoop.h"

namespace dirtbag {

struct FirstAscentDials {
  // Hours with a brush, per unit of cleanliness. At 3.5, taking a line from
  // untouched to merely workable costs about 1.75 hours and getting it
  // properly clean about 3.3 — comparable to a whole session, paid up front
  // and before any chance of success, which is what makes committing to a
  // project a decision rather than a default. It also lands well against
  // the window: brush through the greasy middle of the day, climb when the
  // rock comes into condition.
  double hoursToClean = 3.5;

  // Cleaning is work: not shift-work tiring, but not free either.
  double energyPerHour = 5.0;

  // Below this the line is not worth pulling on, and the game will say so
  // rather than letting you waste skin discovering it.
  double workableCleanliness = 0.55;

  // A brand-new line, before anyone touches it.
  double virginCleanliness = 0.05;

  // Rock does not stay clean. Between sessions a line gives a little back to
  // the weather — enough that a project left for a season needs a re-brush,
  // not enough to punish a normal projecting rhythm.
  double dirtPerNight = 0.02;
};

// What the ledger for an untouched line starts as. Projects begin filthy;
// everything else in the book is clean because people climb it.
ProjectMemory NewProjectLedger(const CragLine& line,
                               const FirstAscentDials& dials =
                                   FirstAscentDials{});

// An hour on the brush. Returns cleanliness gained; costs time and energy on
// the day, so cleaning competes with climbing for the same afternoon.
double CleanLine(PlayerState& player, DayState& day, ProjectMemory& memory,
                 double hours,
                 const FirstAscentDials& dials = FirstAscentDials{},
                 const DayDials& dayDials = DayDials{});

// Whether it is worth pulling on yet, and the reason in the game's voice.
bool IsWorkable(const ProjectMemory& memory,
                const FirstAscentDials& dials = FirstAscentDials{});
std::string CleanlinessText(const ProjectMemory& memory,
                            const FirstAscentDials& dials =
                                FirstAscentDials{});

// True when this line is an unclimbed project that the player has just sent
// and has therefore earned the right to name.
bool CanName(const CragLine& line, const ProjectMemory& memory);

// Name it. Records the given name, the first-ascent claim, and the grade the
// line turned out to be — which is the first time anybody has known it.
// Returns false if the naming was not the player's to do, or the name is
// empty. Never touches memory.routeName: that is the ledger's key.
bool NameFirstAscent(ProjectMemory& memory, const CragLine& line,
                     const std::string& name);

// Tell the scene. Kept separate from NameFirstAscent so that naming stays a
// pure operation on the ledger — this is the part that has opinions, and it
// reads the style off the ledger rather than being told.
void CreditFirstAscent(PlayerState& player, const ProjectMemory& memory);

// The line as the book will print it after the ascent, including who did it
// and what it really went at.
std::string FirstAscentLine(const ProjectMemory& memory,
                            const std::string& by);

// Write the ledger back into the book.
//
// The crag is generated from the world seed and none of it is saved, so a
// first ascent lives only in ProjectMemory. Without this the page never
// changes: the line stays an unclimbed project called "the arete left of
// Diesel" forever, no matter what you did on it or what you called it.
//
// Returns true if the line changed. Idempotent, and safe on a book nobody
// has touched — call it every time the crag is built and again the moment a
// line is named.
bool WriteIntoTheBook(CragLine& line, const ProjectMemory& memory,
                      const std::string& by);

// Overnight upkeep for every project ledger: rock re-dirties slowly.
//
// Call this alongside SleepToNextDay, not from inside it: this layer sits
// above the day loop and inverting that would drag the crag into DirtbagDay.
// Engine-side it lives in the SleepToNextDay node, so the one thing that
// means "a night" runs both halves and no caller has to remember.
void WeatherProjects(PlayerState& player,
                     const FirstAscentDials& dials = FirstAscentDials{});

// Every first ascent this player owns, hardest first — the part of a career
// nobody can take away.
std::vector<const ProjectMemory*> FirstAscents(const PlayerState& player);

}  // namespace dirtbag

// A crag — the guidebook, as data.
//
// Not a gym board. A board is a clean ladder that resets every few weeks and
// whose sandbags are a setter's slip of the wrist; a crag is a fixed
// guidebook where the grades cluster wherever the rock happened to be
// climbable, the sandbags are specific and famous, and some lines have never
// gone at all. That difference is the content, so the lines are authored
// here rather than generated from a distribution — the moves still come from
// the seeded generator, but which lines exist and what the book says about
// them is written down.
//
// The open projects are the point of the whole phase: unnamed, ungraded,
// waiting. Cleaning, working, sending and naming one is Phase 2's gate.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagConditions.h"
#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// One line in the book. A Route plus what the guidebook says about it —
// and, for a project, what nobody can say about it yet.
struct CragLine {
  Route route;

  // Guidebook quality, 0..3. Three stars is why the crag is in the book at
  // all; a no-star line is filler you climb because it is next to the good
  // one. Drives what a player is drawn to, and later what a partner suggests.
  int stars = 0;

  // An unclimbed line: no name worth printing, no grade anyone can vouch
  // for. `route.grade` on a project is the guidebook's guess, flagged as
  // such, and it is allowed to be wrong in a way a graded line is not.
  bool isProject = false;

  // Who did it first. Empty on an open project — filling this in is the
  // first-ascent pipeline (clean -> work -> send -> name), and the reason
  // the field exists before the verb does.
  std::string firstAscentBy;

  // What to call it, when that differs from its identity. Naming a first
  // ascent sets this and never touches `route.name`, because `route.name` is
  // the ledger key: ProjectMemory records burns against it, the save file
  // stores it, and renaming the line out from under a career would orphan
  // every attempt the player had already put into it. The book shows this;
  // the accounting uses route.name forever.
  std::string displayName;

  // How the line is described when it has no name yet: "the arete left of
  // Diesel". Projects are talked about by where they are, because that is
  // all anyone has.
  std::string description;
};

struct Crag {
  std::string name;
  Aspect aspect = Aspect::North;
  std::vector<CragLine> lines;

  // How long the drive is from the van, in hours. The crag's real cost.
  double approachHours = 0.5;
};

// The one crag Phase 2 ships: a roadside boulder field, east-facing, which
// means it bakes at breakfast and comes into the shade mid-afternoon.
Crag RoadsideCrag(const Rng& worldRng);

// The valley's rope crag: a steep north-facing cave forty minutes up the
// hill. North-facing is the whole point of it — Roadside is east-facing and
// bakes all morning, so in high summer, when the season model puts the
// window at dawn and nowhere else, this is the only rock worth walking to.
// It costs the approach and it costs a belayer, and in July it is the only
// climbing there is.
Crag ShadedCave(const Rng& worldRng);

// Lines at or under this grade, in book order — what a guidebook page shows.
std::vector<const CragLine*> LinesUpTo(const Crag& crag, int grade);

// The open projects, in book order.
std::vector<const CragLine*> OpenProjects(const Crag& crag);

// "Diesel V5 ***" / "project, the arete left of Diesel". The guidebook line
// as it would be read aloud.
std::string GuidebookLine(const CragLine& line);

// What to call this line on screen — its given name once it has one, its
// permanent identity otherwise. Never use this as a key.
const std::string& DisplayName(const CragLine& line);

}  // namespace dirtbag

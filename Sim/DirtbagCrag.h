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

  // How long the drive is from the van, in hours. The crag's real cost,
  // and since 2026-08-21 an actual one: the travel spot asks the guidebook
  // for this rather than carrying its own hand-typed copy, so a crag cannot
  // be forty minutes away in the book and half an hour away in the level.
  //
  // Charged on whichever end of a drive is rock, so leaving costs what
  // arriving did.
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

// The valley's third crag: a south-facing boulder terrace, high and cold,
// and the only rock here that is genuinely better in January than in June.
//
// Its identity came out of a measurement rather than folklore. Across a
// year, aspect does almost nothing in summer — North, South and West give
// an identical 25 days at an identical 0.880 friction, because the summer
// window lands at 5:30am before the sun is on any face. It does a great
// deal in winter: South gets 42 days against North's 28, in a window less
// than half as long, landing at about 1:45pm.
//
// Those winter windows land at about 1:45pm, which is where a job bites.
// Stated precisely, because the punchier version is false: over the first
// ninety days a salaried climber can reach 22 of the terrace's 63 windows
// and 13 of the cave's 48 — so the terrace is *not* harder to climb around
// a job than the cave. What it is, is where a job costs you the most: **41
// windows lost against the cave's 35**, the largest number of good days in
// the valley that a nine-to-five takes off you.
Crag SunTerrace(const Rng& worldRng);

// Lines at or under this grade, in book order — what a guidebook page shows.
// unwired-ok: the guidebook page's filter; there is no guidebook screen
std::vector<const CragLine*> LinesUpTo(const Crag& crag, int grade);

// The open projects, in book order.
// unwired-ok: the guidebook page's filter; the engine walks Crag.Lines
// itself
// probe-only: the probe picks a project to work from this; the guidebook
// screen filters the mirrored Crag.Lines it already holds rather than
// converting the whole crag back to sim types to ask. Same answer, and no
// balance rule lives in it.
std::vector<const CragLine*> OpenProjects(const Crag& crag);

// "Diesel V5 ***" / "project, the arete left of Diesel". The guidebook line
// as it would be read aloud.
std::string GuidebookLine(const CragLine& line);

// What to call this line on screen — its given name once it has one, its
// permanent identity otherwise. Never use this as a key.
const std::string& DisplayName(const CragLine& line);

}  // namespace dirtbag

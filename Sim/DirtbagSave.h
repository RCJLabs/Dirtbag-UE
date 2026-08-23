// The first save file — versioned from day one, migration registry from day
// one, exactly as CLAUDE.md demands. Engine-free: serialization is to/from a
// plain string (line-based key=value), so the harness round-trips it and the
// engine wrapper only ever writes bytes to disk.
//
// The contract: old saves must always load. Any change to the save shape
// bumps kSaveVersion and ships a migration in DefaultMigrations() in the
// same commit — never a silent format change.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "DirtbagDay.h"
#include "DirtbagLegacy.h"

namespace dirtbag {

// Version 1: seed + career state (climber, cash, day, project ledgers).
//   DayState is deliberately absent — saves happen at day boundaries.
// Version 2: project ledgers record the route's guidebook grade, so a
//   career can answer "what do you climb?" without the route still
//   existing. v1 ledgers migrate to grade -1 (unknown) rather than a guess.
// Version 3: ledgers carry the first-ascent record — how clean the line is,
//   the name you gave it, whether the first ascent was yours, and the grade
//   it turned out to be. v2 careers only ever touched lines that were
//   already in a book, so they migrate to clean, unnamed, nobody's, and
//   unconfirmed: an exact answer rather than a guess.
// Version 4: who you know at the Lot — rapport, and which lines they got to
//   first. Nobody's strength is stored, because it is derived from the seed
//   and the date. A v3 career knew nobody, which is exactly what an absent
//   list means, so this migration writes a count of zero and nothing else.
// Version 5: the dog. A v4 career had not met it, so it migrates to the
//   stray it starts as — unadopted, unbonded, and hungry.
// Version 6: what you own — shoes, and the van's six parts. A v5 career
//   drove and climbed without either wearing out, so both migrate to new:
//   fresh rubber and a van with nothing wrong with it. That is generous
//   rather than exact, and deliberately so — the alternative is inventing
//   damage a player never earned.
// Version 7: what you owe. Bills used to deduct unconditionally and leave
//   cash negative with nothing behind it; the shortfall is now debt. A v6
//   career owed nothing, which is exact rather than generous.
// Version 8: where you stand with the scene, and whether the crag is shut.
//   A v7 career had no scene to stand with, so it migrates to neutral on
//   all four and an open crag — which is where a new career starts anyway.
// Version 21: who your climber is. A v20 career was built before there were
//   four questions to answer, so it migrates to **unbuilt** — which is not a
//   compromise: an unbuilt character is neutral in every lane by design, so
//   an old save loads and plays with exactly the numbers it was measured
//   with. It also means the creation screen does not ambush somebody
//   mid-career; a v20 climber stays the climber they were.
// Version 22: somebody to beat. A v21 career had no rival, so it migrates to
//   **an empty one** -- and `SleepToNextDay` skips a rival with no name, so
//   an old save plays exactly as it did. It gets one the next time a career
//   starts rather than having a stranger appear mid-life.
// Version 23: the line they are on. v22 shipped the rival a day before the
//   race existed; a v22 career migrates to **no race running**, which is the
//   state a career spends most of its time in anyway.
// Version 24: national ranking points. A v23 career never entered a comp,
//   so it migrates to **zero**, which is where a career starts anyway and
//   puts it at the Local tier -- exactly where an unranked climber belongs.
constexpr int kSaveVersion = 24;

struct SaveGame {
  int version = kSaveVersion;
  std::string seed;  // the world's identity; every RNG stream derives from it
  PlayerState player;

  // The ones that came before, oldest first. This is the whole of what
  // survives a retirement: the world remembers, the body does not. A save
  // with three of these is somebody's grandchild climbing past three
  // generations of family names in the guidebook.
  std::vector<Legacy> legacies;
};

using SaveFields = std::map<std::string, std::string>;

// A migration upgrades the raw field map by exactly one version.
// DefaultMigrations()[i] takes a version (i+1) map to version (i+2).
using Migration = void (*)(SaveFields&);
const std::vector<Migration>& DefaultMigrations();

enum class LoadResult {
  Ok,
  BadFormat,      // not a save, or a field refused to parse
  FutureVersion,  // newer than this build — never guess, never truncate
};

// Deterministic: the same SaveGame always yields the identical string, and
// doubles round-trip exactly (printed at full precision).
std::string SerializeSave(const SaveGame& save);

LoadResult DeserializeSave(const std::string& text, SaveGame& out,
                           const std::vector<Migration>& migrations =
                               DefaultMigrations());

// Exposed for tests and for future migrations' own tests: apply the registry
// to a raw field map from `fromVersion` up to `toVersion`.
bool ApplyMigrations(SaveFields& fields, int fromVersion, int toVersion,
                     const std::vector<Migration>& migrations);

}  // namespace dirtbag

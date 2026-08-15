// Seeded deterministic randomness for the Dirtbag sim core.
//
// Plain C++ on purpose: this translation unit must compile identically inside
// the Unreal module and in the standalone g++ harness (Sim/run-tests.sh), the
// same way landnam-ue proved out. No engine types, no engine RNG — the same
// seed must always produce the same session, day, and world.
//
// mulberry32 seeded by FNV-1a over UTF-16 code units, matching the Landnám
// generator bit-for-bit. Hashing UTF-16 code units (not bytes, not code
// points) is deliberate: it is what a JS reference implementation would do,
// and it is the trap the landnam-ue port already paid to find. Non-ASCII
// seeds ("Þórr", "😀") are covered by tests for exactly that reason.

#pragma once

#include <cstdint>
#include <string>

namespace dirtbag {

// Named streams keep one system's rolls from shifting another's. Adding a
// stream is always safe; renaming one silently changes every existing seed.
enum class Stream { Worldgen, Session, Events };

const char* StreamName(Stream stream);

// FNV-1a over the UTF-16 code units of a UTF-8 string.
uint32_t HashSeedString(const std::string& utf8);

struct Rng {
  std::string seed;   // kept so Derive() can salt sub-streams
  uint32_t state = 0;

  static Rng FromSeed(const std::string& seed);
  static Rng FromStream(const std::string& seed, Stream stream);

  // Uniform in [0, 1). mulberry32; all arithmetic wraps at 32 bits.
  double NextDouble();

  // Uniform integer in [min, max], inclusive.
  int IntRange(int min, int max);

  double FloatRange(double min, double max);
  bool Chance(double p);

  // A fresh generator salted with a label — one per route, per day, per
  // attempt — so systems never share a draw sequence.
  Rng Derive(const std::string& label) const;
};

}  // namespace dirtbag

#include "DirtbagRng.h"

#include <cmath>
#include <vector>

namespace dirtbag {

const char* StreamName(Stream stream) {
  switch (stream) {
    case Stream::Worldgen: return "worldgen";
    case Stream::Session:  return "session";
    case Stream::Events:   return "events";
  }
  return "worldgen";
}

namespace {

// Decode UTF-8 to UTF-16 code units. Surrogate pairs are emitted for
// supplementary-plane characters so that "😀" hashes as two units — a
// code-point loop here would be the exact bug the emoji test exists to catch.
std::vector<uint16_t> Utf8ToUtf16Units(const std::string& utf8) {
  std::vector<uint16_t> units;
  units.reserve(utf8.size());
  size_t i = 0;
  const size_t n = utf8.size();
  while (i < n) {
    const uint8_t b0 = static_cast<uint8_t>(utf8[i]);
    uint32_t cp = 0;
    size_t len = 1;
    if (b0 < 0x80) {
      cp = b0;
    } else if ((b0 & 0xE0) == 0xC0 && i + 1 < n) {
      cp = (b0 & 0x1Fu) << 6 | (static_cast<uint8_t>(utf8[i + 1]) & 0x3Fu);
      len = 2;
    } else if ((b0 & 0xF0) == 0xE0 && i + 2 < n) {
      cp = (b0 & 0x0Fu) << 12 | (static_cast<uint8_t>(utf8[i + 1]) & 0x3Fu) << 6 |
           (static_cast<uint8_t>(utf8[i + 2]) & 0x3Fu);
      len = 3;
    } else if ((b0 & 0xF8) == 0xF0 && i + 3 < n) {
      cp = (b0 & 0x07u) << 18 | (static_cast<uint8_t>(utf8[i + 1]) & 0x3Fu) << 12 |
           (static_cast<uint8_t>(utf8[i + 2]) & 0x3Fu) << 6 |
           (static_cast<uint8_t>(utf8[i + 3]) & 0x3Fu);
      len = 4;
    } else {
      cp = 0xFFFD;  // malformed byte: replacement character, keep going
    }
    if (cp >= 0x10000) {
      const uint32_t v = cp - 0x10000;
      units.push_back(static_cast<uint16_t>(0xD800 + (v >> 10)));
      units.push_back(static_cast<uint16_t>(0xDC00 + (v & 0x3FF)));
    } else {
      units.push_back(static_cast<uint16_t>(cp));
    }
    i += len;
  }
  return units;
}

}  // namespace

uint32_t HashSeedString(const std::string& utf8) {
  uint32_t hash = 0x811c9dc5u;
  for (const uint16_t unit : Utf8ToUtf16Units(utf8)) {
    hash ^= unit;
    hash *= 0x01000193u;  // wraps at 32 bits, like Math.imul
  }
  return hash;
}

Rng Rng::FromSeed(const std::string& seed) {
  Rng rng;
  rng.seed = seed;
  rng.state = HashSeedString(seed);
  return rng;
}

Rng Rng::FromStream(const std::string& seed, Stream stream) {
  return FromSeed(seed + "#" + StreamName(stream));
}

double Rng::NextDouble() {
  // mulberry32. uint32 arithmetic wraps exactly as the reference does —
  // note the parentheses on (1u | state): the missing pair was a real
  // landnam-ue porting bug, kept here as a warning.
  state = state + 0x6d2b79f5u;
  uint32_t t = (state ^ (state >> 15)) * (1u | state);
  t = ((t + ((t ^ (t >> 7)) * (61u | t))) ^ t);
  return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
}

int Rng::IntRange(int min, int max) {
  return min + static_cast<int>(std::floor(NextDouble() * (static_cast<double>(max) - min + 1.0)));
}

double Rng::FloatRange(double min, double max) {
  return min + NextDouble() * (max - min);
}

bool Rng::Chance(double p) {
  return NextDouble() < p;
}

Rng Rng::Derive(const std::string& label) const {
  return FromSeed(seed + "::" + label);
}

}  // namespace dirtbag

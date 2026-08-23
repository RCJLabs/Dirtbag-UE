#include "DirtbagZones.h"

namespace dirtbag {
namespace {

// The grid, as four neighbours per zone. `kNone` is the edge of the map.
//
// Written as a table rather than as coordinates on purpose: the 2D game
// stores it this way (`ZONES` in `world.ts` is four optional links per
// zone), the enum's numbering is arrival order rather than geography, and a
// table is the only form in which "Old Town's east is downtown" can be read
// off and checked by eye. Coordinates would have to be kept in agreement
// with an ordinal that deliberately means nothing.
constexpr int kNone = -1;

struct Links {
  int north, south, east, west;
};

constexpr int Z(Zone z) { return static_cast<int>(z); }

// Indexed by zone ordinal, which is why the off-grid entries have to be
// present and empty rather than omitted.
constexpr Links kGrid[kZoneCount] = {
    /* Lot        */ {Z(Zone::OldTown), Z(Zone::Park), Z(Zone::Trailhead), kNone},
    /* Town        */ {Z(Zone::Uptown), Z(Zone::Trailhead), Z(Zone::Midtown), Z(Zone::OldTown)},
    /* Roadside    */ {kNone, kNone, kNone, kNone},
    /* Cave        */ {kNone, kNone, kNone, kNone},
    /* Terrace     */ {kNone, kNone, kNone, kNone},
    /* OldTown     */ {Z(Zone::MarketRow), Z(Zone::Lot), Z(Zone::Town), kNone},
    /* Midtown     */ {Z(Zone::GrandPlaza), Z(Zone::Outskirts), kNone, Z(Zone::Town)},
    /* Trailhead   */ {Z(Zone::Town), kNone, Z(Zone::Outskirts), Z(Zone::Lot)},
    /* Outskirts   */ {Z(Zone::Midtown), kNone, kNone, Z(Zone::Trailhead)},
    /* Uptown      */ {kNone, Z(Zone::Town), Z(Zone::GrandPlaza), Z(Zone::MarketRow)},
    /* MarketRow   */ {kNone, Z(Zone::OldTown), Z(Zone::Uptown), kNone},
    /* GrandPlaza  */ {kNone, Z(Zone::Midtown), kNone, Z(Zone::Uptown)},
    /* Park        */ {Z(Zone::Lot), Z(Zone::Lake), kNone, kNone},
    /* Lake        */ {Z(Zone::Park), kNone, kNone, kNone},
    /* Village     */ {kNone, kNone, kNone, kNone},
    /* Farm        */ {kNone, kNone, kNone, kNone},
};

bool InRange(Zone z) {
  const int i = Z(z);
  return i >= 0 && i < kZoneCount;
}

}  // namespace

bool NeedsTheVan(Zone zone) {
  // Read off the grid rather than listed. A zone with no walkable border is
  // one you drive to, by definition, and stating it once means a zone added
  // later cannot be walkable in the table and drivable in this function --
  // which is the shape of every bug this file's append-only rule is about.
  if (!InRange(zone)) return true;
  const Links& l = kGrid[Z(zone)];
  return l.north == kNone && l.south == kNone && l.east == kNone &&
         l.west == kNone;
}

bool IsConnectedGround(Zone zone) { return !NeedsTheVan(zone); }

bool NeighbourOf(Zone from, Compass dir, Zone& out) {
  if (!InRange(from)) return false;
  const Links& l = kGrid[Z(from)];
  int to = kNone;
  switch (dir) {
    case Compass::North: to = l.north; break;
    case Compass::South: to = l.south; break;
    case Compass::East:  to = l.east;  break;
    case Compass::West:  to = l.west;  break;
  }
  if (to == kNone) return false;
  out = static_cast<Zone>(to);
  return true;
}

bool Adjacent(Zone a, Zone b) {
  if (a == b) return false;
  if (!InRange(a) || !InRange(b)) return false;
  const int t = Z(b);
  const Links& l = kGrid[Z(a)];
  return l.north == t || l.south == t || l.east == t || l.west == t;
}

int WalkCrossings(Zone from, Zone to) {
  if (!InRange(from) || !InRange(to)) return -1;
  if (from == to) return 0;
  if (NeedsTheVan(from) || NeedsTheVan(to)) return -1;

  // Breadth-first over sixteen nodes. Deliberately not Dijkstra: every
  // border costs the same, because in the 2D game crossing one is walking
  // off the side of the screen and there is no such thing as a long edge.
  // If a border ever costs more than another, this becomes the wrong
  // algorithm and should be changed rather than weighted in the caller.
  int dist[kZoneCount];
  for (int i = 0; i < kZoneCount; i++) dist[i] = -1;
  int queue[kZoneCount];
  int head = 0, tail = 0;
  dist[Z(from)] = 0;
  queue[tail++] = Z(from);
  while (head < tail) {
    const int here = queue[head++];
    if (here == Z(to)) return dist[here];
    const Links& l = kGrid[here];
    const int next[4] = {l.north, l.south, l.east, l.west};
    for (int k = 0; k < 4; k++) {
      if (next[k] == kNone || dist[next[k]] >= 0) continue;
      dist[next[k]] = dist[here] + 1;
      queue[tail++] = next[k];
    }
  }
  // Unreachable on connected ground would mean the table has an island in
  // it, which the tests forbid. -1 rather than an assert: a sim that
  // refuses to answer is worse than one that says "you cannot walk that".
  return -1;
}

double WalkMinutes(Zone from, Zone to, const ZoneDials& dials) {
  const int crossings = WalkCrossings(from, to);
  if (crossings < 0) return -1.0;
  return crossings * dials.minutesPerCrossing;
}

bool IsACrag(Zone zone) {
  // Named rather than derived from `NeedsTheVan`, and the reason arrived
  // exactly as predicted: the first version of this file said the two were
  // "equivalent now, correct later, the moment comps exist", and comps came
  // back on 2026-08-23. The Village needs the van and is not rock. Neither
  // is the farm.
  return zone == Zone::Roadside || zone == Zone::Cave ||
         zone == Zone::Terrace;
}

const char* ZoneName(Zone zone) {
  switch (zone) {
    case Zone::Lot:        return "the Lot";
    // "town" was right while there was one of them. There are seven now, so
    // the enum keeps its name -- renumbering is what this file must never
    // do -- and the word a player reads becomes the district's.
    case Zone::Town:       return "downtown";
    case Zone::Roadside:   return "Roadside";
    case Zone::Cave:       return "the Shaded Cave";
    case Zone::Terrace:    return "the Sun Terrace";
    case Zone::OldTown:    return "Old Town";
    case Zone::Midtown:    return "Midtown";
    case Zone::Trailhead:  return "the Trailhead";
    case Zone::Outskirts:  return "the Outskirts";
    case Zone::Uptown:     return "Uptown";
    case Zone::MarketRow:  return "Market Row";
    case Zone::GrandPlaza: return "Grand Plaza";
    case Zone::Park:       return "Greenwood Park";
    case Zone::Lake:       return "Trout Lake";
    case Zone::Village:    return "the Olympic Village";
    default:               return "the farm";
  }
}

}  // namespace dirtbag

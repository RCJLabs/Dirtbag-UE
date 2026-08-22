#include "DirtbagZones.h"

namespace dirtbag {

bool NeedsTheVan(Zone zone) {
  switch (zone) {
    case Zone::Lot:
    case Zone::Town:
      return false;
    default:
      // Crags, and comps when they exist. Written as a default rather than
      // as a list so that a zone added later needs the van until somebody
      // deliberately says otherwise -- the safe direction to fail, because
      // a crag you can walk to breaks the economy and a town you have to
      // drive to only annoys.
      return true;
  }
}

bool IsACrag(Zone zone) {
  // Named rather than written as "everything that needs the van", which is
  // the same answer today and stops being the same answer the moment comps
  // exist: a comp needs the van and is not rock. Equivalent now, correct
  // later, and the tests say so out loud rather than implying a guard they
  // do not yet provide.
  return zone == Zone::Roadside || zone == Zone::Cave ||
         zone == Zone::Terrace;
}

double WalkMinutes(Zone from, Zone to, const ZoneDials& dials) {
  if (from == to) return 0.0;
  if (NeedsTheVan(from) || NeedsTheVan(to)) return -1.0;
  // Only two zones are on connected ground today, so there is exactly one
  // walk. When the town grows this becomes a table; it is not one yet, and
  // a table of two entries is a table nobody maintains.
  return dials.lotToTownMinutes;
}

const char* ZoneName(Zone zone) {
  switch (zone) {
    case Zone::Lot:
      return "the Lot";
    case Zone::Town:
      return "town";
    case Zone::Roadside:
      return "Roadside";
    case Zone::Cave:
      return "the Shaded Cave";
    default:
      return "the Sun Terrace";
  }
}

}  // namespace dirtbag

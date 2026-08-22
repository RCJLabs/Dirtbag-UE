#pragma once

// Where you are in the world, and whether you can walk there.
//
// **The 2D game is the spec, and this file exists because the port had
// quietly departed from it.** Evan, 2026-08-22:
//
//   *"there are supposed to be attached zones for each location. the
//   original dirtbag game that was made had zones connected except for
//   crags and the olympics which you needed to use the van to get to."*
//
// The port had no concept of a zone at all. `Venue` answers a different
// question -- *what rock am I on* -- and it only knows Gym, Crag, Cave and
// Terrace, so the Lot and the town were nowhere in the model. Every
// destination was reached by the same travel spot, and **every travel spot
// was gated on the van running.**
//
// That is one rule where the 2D game has two, and the difference matters
// more than it looks. In the 2D game a dead van means *no climbing*; here
// it meant **no climbing, no gym, no shop, no shift and no diner** -- the
// van breaking took the whole game away instead of taking the crags away.
// The pressure is supposed to land as "stuck in town earning the money to
// fix it", which is a week of the game, not "stuck".
//
// Two tiers, and that is the whole model:
//
//   walk   the Lot, the town  -- connected ground, you can see one from
//                               the other, no van involved ever
//   drive  Roadside, the Cave, the Terrace, and comps when they exist
//
// Kept deliberately separate from `Venue`. A zone is *where in the world*
// and a venue is *what rock*: the Town zone contains the Gym venue plus
// three destinations that are not climbing at all, and collapsing the two
// is what would put the diner on the guidebook.
//
// Engine-free like everything in Sim/.

#include <string>

namespace dirtbag {

enum class Zone {
  Lot = 0,      // the van, the fire, the dog, the three regulars
  Town,         // the gym, the gear shop, the diner, the job
  Roadside,     // crag 1
  Cave,         // crag 2 -- north-facing, forty minutes up the hill
  Terrace,      // crag 3 -- south-facing, high and cold
};
constexpr int kZoneCount = 5;

struct ZoneDials {
  // Minutes on foot between the Lot and town.
  //
  // Twenty because it has to be worth the van when the van runs and
  // survivable when it does not -- a walk that costs a whole morning is a
  // punishment, and one that costs nothing makes the van pointless for
  // anything but rock.
  double lotToTownMinutes = 20.0;
};

// Can you get there on your own legs? The crags cannot; everything on
// connected ground can.
bool NeedsTheVan(Zone zone);

// Minutes on foot, or -1 when you cannot walk it at all. Walking to
// yourself is free rather than an error -- pressing "go" where you already
// are should cost nothing, not assert.
double WalkMinutes(Zone from, Zone to, const ZoneDials& dials = ZoneDials{});

// Is this zone rock you climb on? The Gym is climbing and is in Town, so
// this is not "not the Lot and not the Town" and must not be written that
// way.
bool IsACrag(Zone zone);

const char* ZoneName(Zone zone);

}  // namespace dirtbag

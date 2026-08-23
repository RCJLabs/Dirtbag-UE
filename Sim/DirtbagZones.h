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
//   drive  Roadside, the Cave, the Terrace, the Village, the farm
//
// Kept deliberately separate from `Venue`. A zone is *where in the world*
// and a venue is *what rock*: downtown holds the Gym venue plus three
// destinations that are not climbing at all, and collapsing the two is what
// would put the diner on the guidebook.
//
// ---------------------------------------------------------------------
// **Widened 2026-08-23 from five zones to sixteen** (`notes/the-2d-audit.md`).
// The first version of this file was built from Evan's description, and the
// description was right about the *rule* and short about the *map*: the 2D
// game's `world.ts` has **eleven walkable zones in one connected grid**,
// plus Notown, four walk-in interiors and three van-only destinations, and
// this file had two walkable zones and three crags.
//
// Three decisions that are load-bearing enough to write down:
//
// **Append-only, and this is not a style preference.** `EDirtbagZone` is a
// `uint8` and `ADirtbagDaySpot::DestinationZone` is an `EditAnywhere`
// property, so every travel spot already placed in the level stores its
// destination *by ordinal*. Inserting a zone anywhere before the end
// silently repoints all of them -- a level-wide bug with no compiler error
// and no test failure, discovered by walking to the gym and arriving at a
// crag. So Lot..Terrace keep 0..4 forever and everything new lands after,
// even where that leaves the enum in a daft order. **The grid is in the
// adjacency table, not in the numbering.**
//
// **Interiors are not zones here.** The 2D game makes the cafe, the diner,
// the cabin and the mall their own `ZoneId`s, but everything that hangs off
// that is presentation -- `isInterior`, `interiorZoom`, a camera pull. The
// sim already models the shop counter and the diner as *spots*, which is
// the right grain: walking through a door does not change which zone you
// are in, it changes what the camera does.
//
// **Notown is left out on purpose.** It is on `DIRTBAG.md` §4's cut list
// and has not been reversed. Adding it later is one append at the end,
// which the rule above makes safe by construction -- which is rather the
// point of the rule.
//
// Engine-free like everything in Sim/.

#include <string>

namespace dirtbag {

// The map, laid out. Columns are west/centre/east; the Lot is bottom-left
// and downtown is the middle of it, which is why the walk from one to the
// other is two borders rather than one.
//
//     [ Market Row ][   Uptown  ][ Grand Plaza ]
//     [  Old Town  ][  downtown ][   Midtown   ]
//     [  the Lot   ][ Trailhead ][  Outskirts  ]
//     [ Greenwood  ]
//     [ Trout Lake ]
//
// Roadside, the Cave, the Terrace, the Olympic Village and the farm sit off
// the grid entirely. You do not walk to any of them.
enum class Zone {
  // ---- 0..4: the original five. Never renumber these. ----
  Lot = 0,      // the van, the fire, the dog, the three regulars
  Town,         // the gym, the gear shop, the diner, the job -- "downtown"
  Roadside,     // crag 1
  Cave,         // crag 2 -- north-facing, forty minutes up the hill
  Terrace,      // crag 3 -- south-facing, high and cold

  // ---- 5+: appended 2026-08-23. Order is arrival, not geography. ----
  OldTown,      // west of downtown, north of the Lot -- the old blocks
  Midtown,      // east of downtown
  Trailhead,    // south of downtown, where the dirt road starts
  Outskirts,    // south-east, the cheap end
  Uptown,       // north of downtown, civic
  MarketRow,    // north-west, the retail strip
  GrandPlaza,   // north-east, the expensive square
  Park,         // south of the Lot, grass to grass, no road between
  Lake,         // south of the park -- the quiet end of the map
  Village,      // the Olympic Village. Van only, and not rock.
  Farm,         // your folks' place. Van only, and not rock.
};
constexpr int kZoneCount = 16;

// Which way you walked off the edge. The 2D game's movement rule exactly:
// you do not pick a destination on connected ground, you walk until the
// screen changes.
enum class Compass { North, South, East, West };

struct ZoneDials {
  // Minutes on foot to cross one zone border.
  //
  // **Ten, and the ten is derived rather than chosen.** This dial replaced
  // `lotToTownMinutes = 20.0` when the map widened, and twenty was a tuned
  // number: long enough to be worth the van when the van runs, short enough
  // to survive when it does not -- a walk that costs a whole morning is a
  // punishment and one that costs nothing makes the van pointless for
  // anything but rock.
  //
  // On the real grid the Lot and downtown are diagonal, so that walk is
  // **two borders** either way round (via Old Town or via the Trailhead).
  // Ten a border keeps the one walk anybody has tuned at exactly the twenty
  // minutes it was tuned to, and prices every new walk off it rather than
  // off a guess. Widening the map must not quietly rebalance the one route
  // that was already right.
  double minutesPerCrossing = 10.0;
};

// Can you get there on your own legs? The crags cannot, nor the Village,
// nor the farm; everything on connected ground can.
bool NeedsTheVan(Zone zone);

// The same question asked the way it reads at a call site. `!NeedsTheVan`
// is a double negative in every sentence that wants it.
// unwired-ok: the engine asks NeedsTheVan directly, which is the same
// answer. This exists so sim-side callers read forwards; wrapping it for
// Blueprint as well would be two names for one rule, which is how two
// copies of a rule start disagreeing.
bool IsConnectedGround(Zone zone);

// Do these two share a border? False for anything off the grid, and false
// for a zone against itself -- you are not next to where you are standing.
// unwired-ok: Phase 6. Nothing in the game walks off the edge of a zone
// yet -- travel is still a fade and a `TeleportTo` -- so a Blueprint
// wrapper here would be a verb with no door, which is this project's
// favourite bug. It gets wired when connected ground is built and the
// player can walk from the Lot to Old Town without choosing to.
bool Adjacent(Zone a, Zone b);

// Walk off the edge in a direction. Returns false when there is nothing
// that way (the map has edges) or when `from` is off the grid, and leaves
// `out` untouched when it does.
// unwired-ok: Phase 6, same as `Adjacent` -- this is the verb connected
// ground is built on and there is no connected ground yet. It is not
// dead in the meantime: `TestZones` walks every link from both ends
// through it, which is what proves the adjacency table has no one-way
// streets in it.
bool NeighbourOf(Zone from, Compass dir, Zone& out);

// How many borders between them on foot: 0 for a zone against itself, -1
// when either end needs the van. The grid is connected, so any two walkable
// zones always have an answer.
int WalkCrossings(Zone from, Zone to);

// Minutes on foot, or -1 when you cannot walk it at all. Walking to
// yourself is free rather than an error -- pressing "go" where you already
// are should cost nothing, not assert.
double WalkMinutes(Zone from, Zone to, const ZoneDials& dials = ZoneDials{});

// Is this zone rock you climb on? The Gym is climbing and is downtown, so
// this is not "not the Lot and not the town" and must not be written that
// way -- and since 2026-08-23 it is not "everything that needs the van"
// either, because the Village needs the van and is a competition, and the
// farm needs the van and is your mother.
bool IsACrag(Zone zone);

const char* ZoneName(Zone zone);

}  // namespace dirtbag

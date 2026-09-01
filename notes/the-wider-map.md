# The wider map — five zones to sixteen

**2026-08-23.** Evan: *"then the zone graph"*, after the audit found the
port's world model was a sixth the size of the 2D game's.

`Sim/DirtbagZones.h` was built on 2026-08-22 from Evan's description of how
travel works. The description was **right about the rule and short about the
map**: two tiers, walk vs van, exactly as the 2D game does it — and two
walkable zones where `world.ts` has eleven.

## What the map actually is

    [ Market Row ][   Uptown  ][ Grand Plaza ]
    [  Old Town  ][  downtown ][   Midtown   ]
    [  the Lot   ][ Trailhead ][  Outskirts  ]
    [ Greenwood  ]
    [ Trout Lake ]

Eleven walkable zones in one connected grid, with a stem of park and lake
running south off the Lot. **Off the grid entirely:** Roadside, the Shaded
Cave, the Sun Terrace, the Olympic Village and your folks' farm. Sixteen
zones, and the two-tier rule unchanged — *"zones connected except for crags
and the olympics which you needed to use the van to get to."*

Notown is left out. It is on §4's cut list and has not been reversed.

## Three decisions worth the words

**Append-only, and it is not a style preference.** `EDirtbagZone` is a
`uint8` and `ADirtbagDaySpot::DestinationZone` is an `EditAnywhere`
property, so **every travel spot already placed in Evan's level stores its
destination as a number.** Inserting a zone anywhere but the end renumbers
all of them at once: no compiler error, no failing test anywhere else, and
the symptom is walking to the gym and arriving at a crag. So `Lot..Terrace`
keep 0..4 forever and the grid lives in the adjacency table rather than in
the numbering — which is why the enum reads in arrival order and not in
geography. `TestZones` pins the five ordinals, and the mirror carries the
same warning.

**Interiors are not zones here.** The 2D game gives the cafe, the diner, the
cabin and the mall their own `ZoneId`s, but everything hanging off that is
presentation — `isInterior`, `interiorZoom`, a camera pull. The sim already
models the shop counter and the diner as *spots*, which is the right grain:
walking through a door does not change which zone you are in, it changes
what the camera does.

**The tuned number survived the widening, and that took arithmetic.**
`lotToTownMinutes = 20.0` was a tuned dial — long enough to be worth the van
when the van runs, short enough to survive when it does not. On the real
grid **the Lot and downtown are diagonal**, so that walk is two borders
either way round (via Old Town or via the Trailhead). The dial is now
`minutesPerCrossing = 10.0`, chosen so the one walk anybody has ever tuned
still costs exactly the twenty minutes it was tuned to, and so every new
walk is priced off that rather than off a guess. **Widening a map must not
quietly rebalance the route that was already right**, and the only way to
know it did not is to pin it — `WalkMinutes(Lot, Town) == 20.0` is a test
now.

The longest walk on the map is Trout Lake to Grand Plaza: six borders, an
hour on foot. The quiet end to the expensive one, and it should cost that.

## What the tests are for

The adjacency table is hand-typed and **every link in it is written twice,
once from each end.** A table written twice disagrees with itself
eventually, so most of the new tests check my typing rather than a design:

- **Reverse links.** If north of A is B then south of B is A. A link entered
  from one side only is a one-way street you can walk into and not out of.
- **Connected ground is connected.** An island in the table is a zone you
  can see and never reach on foot — and because `WalkCrossings` answers -1
  for *"you cannot walk that"*, an island reads at every call site as though
  it needed the van. This check is what tells the two apart.
- **Distances pinned, not ordered.** *"The lake is further than the
  Trailhead"* passes on a map where everything is one border from
  everything, which is exactly what a bad edit produces. So the numbers are
  pinned: Lot→downtown 2, Lot→Old Town 1, Lake→Grand Plaza 6.
- **Frozen ordinals**, above.

**Verified by reintroduction**, five defects, each put back and counted:

| defect | caught by |
|---|---|
| insert a zone between Lot and Town | **218 failures**, led by the ordinal pins |
| drop Trailhead's link back to the Lot | 15 |
| cut Trout Lake off entirely | 8 |
| write `IsACrag` as `NeedsTheVan` | 3 |
| price a border at 20 instead of 10 | 2, naming both tuned walks |

The first attempt at the renumber test **did not reproduce the defect** and
reported zero: `Lot = 0` is explicitly assigned, so an enumerator inserted
above it collides at 0 rather than shifting anything. A reintroduction that
does not reintroduce is a test that has not been checked, which is the same
trap as a checker that passes because it is not looking.

## The day the comment was written for

`IsACrag` was written out longhand rather than as `NeedsTheVan`, with a note
saying the two were the same set today and that the checks *"cannot
currently fail"* — kept for the day comps arrived, because **a comp needs
the van and is not rock**, and on that day the lazy definition starts
quietly putting a climbing competition on the guidebook.

Comps came back this morning. The Olympic Village landed with them and the
farm came with it. **Both need the van and neither is rock**, so those
checks are live guards now instead of a comment hoping somebody reads it.

## The bug the widening created, and it was silent

`ADirtbagDaySpot::WalkHours` worked out where you were walking **from** by
looking at where you were walking **to**:

```cpp
const EDirtbagZone From = DestinationZone == EDirtbagZone::Town
                              ? EDirtbagZone::Lot
                              : EDirtbagZone::Town;
```

With two walkable zones that is not a hack, it is a proof. With eleven it is
simply false — and **it fails without failing**: the walk still costs a
number, and the number is somebody else's walk. Its own comment had promised
otherwise (*"asked of the zone model rather than typed here: when the town
grows, the table grows in one file and every spot pointing along it
follows"*), which was true of the minutes and not of the origin.

The engine had **no notion of which zone the player was in at all** — with
two zones you never needed one. `UDirtbagGameInstance::CurrentZone` is that
notion: it starts at the Lot, because you wake in the van, and it is set
beside `SetVenue` on arrival, which is the one moment both *where in the
world* and *what rock* change. Set after the hours are computed, since
`WalkHours` reads it as the origin and would otherwise price the walk from
where you are about to be.

**Deliberately not saved yet.** Nothing in the save has ever recorded where
you were standing, and adding it is a SAVE_VERSION bump for a field that
starts mattering when Phase 6 makes zones real places you load back into.
Today a load puts you at the Lot, which is where the level spawns you
anyway, so the two agree. Recorded rather than done quietly.

**This fix has no test and cannot have one from here** — it is engine-side,
there is no Unreal compiler in the container, and the harness cannot reach
`WalkHours`. What is checkable is checked: the sim's side is tested, and a
grep confirms `EDirtbagZone::Town` no longer appears anywhere as an inferred
origin.

## Three functions with no door, recorded rather than wrapped

`IsConnectedGround`, `Adjacent` and `NeighbourOf` are `unwired-ok`.
`NeighbourOf` in particular is the verb connected ground is built on — walk
off the edge of a zone and arrive in the neighbour — and **there is no
connected ground yet**: travel is still a fade and a `TeleportTo`. Wrapping
them for Blueprint now would be three verbs with no door, which is this
project's favourite bug and has been found at six layers already.

They are not dead in the meantime: `TestZones` walks every link from both
ends through `NeighbourOf`, which is what proves the table has no one-way
streets in it. The preflight run prints the unwired count on every
invocation, so they stay visible rather than quietly becoming permanent.

## Where this leaves Phase 6

The topology is data and it is now right. The blockout can start against a
map that will not need re-cutting, which was the whole argument for doing
this before the level rather than after: **an afternoon now, a level
rebuild later.**

Nine of the eleven walkable zones have no content — no spots, no buildings,
nothing to do. That is fine and it is what a real town is like between the
places you go. They get a walk-through and a name until something wants to
live there.

# Phase 6 — what to place, and where

*2026-08-25.* Written from away, from the code, so that a session at the
desk is placing things rather than working out what to place.

`notes/phase6-the-place.md` says **the Lot first**, then Roadside, then
town, and **blockout → walkable → dress, never dress first**. This does not
change that. It is the other half: the *inventory*, so that when a pocket is
walkable you know exactly which actors go in it and what each one unblocks.

**The whole placement surface is four actor classes.** There is no fifth,
and nothing here needs a Blueprint written for it — every one is a C++ class
you drop and set properties on.

| class | what it is |
|---|---|
| `ADirtbagZoneVolume` | a box. Says which zone you are standing in |
| `ADirtbagDaySpot` | everything you walk up to and press a key at |
| `ADirtbagClimbWall` | a wall with a spline of holds |
| `ADirtbagSleepSpot` | lights out |

---

## The critical path: nine spots and the Lot is playable

If you place nothing else this session, place these. A career can be lived
end to end with them — badly, and with a lot missing, but **without getting
stuck**, which is the bar for a blockout.

| # | zone | class · Kind | properties | unblocks |
|---|---|---|---|---|
| 1 | Lot | `DaySpot` · `Sleep` | — | the day advancing at all |
| 2 | Lot | `DaySpot` · `Bivy` | — | where you park; tickets, the boot |
| 3 | Lot | `DaySpot` · `Travel` | `TravelName` "the crag", **`DestinationZone` `Roadside`**, `ArriveAt` `Crag` | leaving |
| 4 | Roadside | `DaySpot` · `Travel` | `TravelName` "the Lot", **`DestinationZone` `Lot`**, `ArriveAt` `Gym` | coming back |
| 5 | Roadside | `ClimbWall` | `Venue` `Crag` | climbing outside |
| 6 | Town | `DaySpot` · `Meal` | — | not starving |
| 7 | Town | `DaySpot` · `Shift` | — | money |
| 8 | Town | `ClimbWall` | `Venue` `Gym`, `BoardIndex` 0 | climbing when it rains |
| 9 | Town | `DaySpot` · `Travel` | `DestinationZone` `Lot`, `ArriveAt` `Gym` | the loop closing |

**A travel spot has two destination properties and they answer different
questions.** Getting this pair wrong is the one way to place a spot that
compiles, runs, and is quietly broken:

- **`DestinationZone`** — where in the world you end up. This is the one
  that decides everything: `NeedsTheVan(DestinationZone)` picks walk or
  drive, so a **walk costs time and nothing else** — no fuel, no wear, no
  breakdown roll — and a drive costs all three. It defaults to `Roadside`,
  which is van-gated, and the default is deliberately the safe direction to
  be wrong in: a spot you forget to set stays a drive rather than silently
  becoming free.
- **`ArriveAt`** — which *rock* you are now standing at, one of `Gym`,
  `Crag`, `Cave`, `Terrace`. There is no Lot venue, so **a spot that takes
  you home sets `DestinationZone` `Lot` and leaves `ArriveAt` at `Gym`** —
  you are not at a wall, and `Gym` is what "indoors, no window" means to
  everything downstream.

`TravelHours` is only read for the drive; a walk is priced from the zone
graph.

**Zone volumes:** one per pocket you have built, `Zone` set to match. A spot
outside every volume still works — the zone is what the *walk* costs and
what the HUD calls the place — but the twenty-minute Lot→downtown walk is
zone-driven, so an unvolumed town is a town you teleport into.

---

## Everything else, by zone, in the order the plan builds them

Each row is one actor. **Nothing below is required for the loop above to
work** — this is the difference between a level you can finish a career in
and a level that is the game.

### The Lot — the most-seen place in the game by an order of magnitude

| class · Kind | properties | note |
|---|---|---|
| `DaySpot` · `Fire` | `StartingStakeNotch` 1 | the one actor `notes/phase2-lot.md` already specified |
| `DaySpot` · `Dog` | — | the bowl by the van |
| `DaySpot` · `Van` | — | under it with a spanner |
| `DaySpot` · `Rest` | `RestHours` 1, `bWaitForWindow` **false** — a guess, see Roadside below | the tailgate |
| `DaySpot` · `Keeping` | `Keeps` `WashInTheVan` | a rag and a jug |
| `DaySpot` · `Keeping` | `Keeps` `Water` | fill the jugs |
| `DaySpot` · `Keeping` | `Keeps` `Propane` | swap the bottle |
| `DaySpot` · `Evening` | `Thread` `Someone` | the phone box |
| `DaySpot` · `Evening` | `Thread` `Home` | also the phone box — same prop, different spot |
| `DaySpot` · `Evening` | `Thread` `Music` | the tailgate with the guitar out |
| `DaySpot` · `Evening` | `Thread` `Books` | the milk crate of paperbacks |
| `DaySpot` · `Evening` | `Thread` `Cooking` | the stove |

**Five Evening spots is five props within a few metres of the van**, and
that is deliberate: the whole of `Sim/DirtbagLife.h` is *which* evening you
spend, so five separate things to walk to is the interface. If it reads as
clutter, they can share one prop and be five collision boxes.

### Roadside — the first crag

| class · Kind | properties |
|---|---|
| `ClimbWall` | `Venue` `Crag`, one per line you want climbable |
| `DaySpot` · `Rest` | `RestHours` 1, `bWaitForWindow` **true** — *this is the one that matters* |
| `DaySpot` · `Travel` | `DestinationZone` `Lot`, `ArriveAt` `Gym` |

**`bWaitForWindow` is the one checkbox here that changes a system rather
than a number.** On, one press waits exactly until the window opens; off,
you sit in fixed hour chunks and watch the forecast move under you. **At a
crag it wants to be on** — the whole conditions model is built around a verb
that waits for the rock, and a crag rest spot with it off throws that away.

At the Lot it is a preference, not a correctness question, and I have
guessed: **off**, so that resting in the van is an hour at a time and
waiting for conditions is something you do at the rock. If that feels wrong
in play, it is one checkbox.

### Town — you already own the building generator

| class · Kind | properties |
|---|---|
| `DaySpot` · `GearShop` | — |
| `DaySpot` · `Gym` | `GymNameToBuy` — the gym you can buy, **eight pages of levers**, see below |
| `DaySpot` · `Keeping` | `Keeps` `TruckStop` — the only wash that gets you properly clean |
| `DaySpot` · `Travel` | `DestinationZone` `Cave`, `ArriveAt` `Cave` |
| `DaySpot` · `Travel` | `DestinationZone` `Terrace`, `ArriveAt` `Terrace` |
| `ClimbWall` | `Venue` `Gym`, `BoardIndex` 0 and 1 — two boards, so a rest day has somewhere to go |

### Trailhead, Lake, and the rest of the sixteen zones

The map was widened to sixteen zones on 2026-08-23 precisely so that this
could be walk-through-and-a-name for now. Two of them have content waiting:

| zone | class · Kind | properties |
|---|---|---|
| Lake | `DaySpot` · `Keeping` | `Keeps` `Lake` — free, an afternoon, a rinse |
| Trailhead | `DaySpot` · `Bivy` | a second one, so parking up the trail is a place and not a menu |

Everything else can stay a corridor.

---

## Two things to judge, which is what a desk session is for

**1. The gym counter is eight pages behind six keys.** Walk up to a gym you
own and cycle key 6:

```
the floor · the levers · the people · the building · the squad ·
the federation · league night · the keys
```

That was the honest way to fit thirty verbs behind the keys that exist
without inventing an input mode, and **I do not think it is right.** It is a
ten-second judgement at the desk and one I cannot make from here. If it
reads badly, say so and it becomes a widget.

**2. `bWaitForWindow` at Roadside, per the section above.** Sit at the crag
in bad conditions and confirm the clock runs forward to the window rather
than by an hour.

---

## What placing all of this does *not* need

- **No Blueprints.** Every class here is C++ with `EditAnywhere`
  properties. Reparenting to a BP for a mesh is fine and changes nothing.
- **No new input bindings.** Keys 1–6 and E are already bound and every
  spot above rides them.
- **No animation work.** `ADirtbagClimbWall`'s six anim slots take the
  `ClimbingAnimationSet` already installed; an unset slot is a warning, not
  a crash.
- **No rock purchase.** A wall is a spline plus a mesh and the rock behind
  it is scenery — `notes/phase6-the-place.md` settled this and it still
  holds.

---

## How to know it worked

The phase's **Done when** is *"you can walk out of the van, drive to a crag,
climb, drive to town, work a shift and sleep — and every one of those
happens somewhere that looks like a place."*

The nine-spot critical path is the first half of that sentence. The rest of
the inventory is what makes the second half worth doing.

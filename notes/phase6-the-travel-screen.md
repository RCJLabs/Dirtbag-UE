# The travel screen

*2026-08-22. The first build of Phase 6, and the second half of the
correction that opened it.*

Evan, describing the 2D game:

> *"when traveling you saw the van moving across a background traveling to
> the crag or whatever else."*

The port had **fade to black, teleport, fade in.**

## Why this is not decoration

A fade says *time passed*. That is all it can say.

**This is the only time the van is ever on screen as a vehicle** rather than
as a thing you sleep in and repair. And the van is what the money is *for*:
it is the buffer a dream costs, the thing that breaks and takes a week off
you, the reason a bad night at the fire is felt twice. Every system in this
game points at it and the player has never once watched it go anywhere.

A van crossing a background says *this is the thing you keep alive*, once
per trip, for free.

It also earns its keep a second way: it is the one place the game can show a
**walk** as something different from a drive — no van on screen, no fuel
line — which is the distinction `Sim/DirtbagZones.h` was built to make and
which otherwise happens entirely in a prompt.

## What it does

Same shape as every readout in this project: the spot fills
`FDirtbagTravelReadout`, the canvas HUD draws it, no Blueprint work.

- **Where you are going**, and whether you are walking or driving.
- **The van**, entering off one edge and leaving off the other — it does not
  start or stop in frame, because a van parked at the screen edge for a beat
  reads as a bug.
- **The clock, running.** A forty-minute approach eats a window, and a
  number that ticks says so better than one that jumps.
- **Fuel**, on a drive only.
- **The breakdown**, if the van gave up — said *on the road*, which is where
  it happened and which the game has never been able to show before.

The road takes the whole screen and everything else stops drawing. You are
between places; there is no decision available, so a pump bar and a shop
prompt over the top would be furniture.

**Any key skips it.** `TravelSeconds` is 2.6 and the first trip is a moment,
but the fiftieth is a keypress and a career has hundreds. A travel screen
you cannot skip stops being a moment and becomes a tax.

## The ordering decision

**The sim's side of the trip is taken once, up front, before a single frame
of road is drawn.** Hours, fuel, wear, the breakdown roll, the venue change
— all of it, then the screen runs as pure presentation and applies nothing.

That is deliberate and it is the safe direction. If the player quits halfway
down the road the world is already in the state of having arrived, which is
consistent. Applying at the end would leave a half-taken trip on the floor.

The consequence is that the clock on screen is **interpolated for display
only** — and wrapped rather than clamped, because a drive leaving at 23:30
and taking forty minutes arrives at 00:10, and a clamp would sit the display
at 23:59 claiming the trip took half an hour longer than it did.

The teleport is the one thing left until the end, so the world behind the
screen only matches where the screen says you are once it says you have
arrived.

## Assets: none, as usual

`RoadBackdrop` and `VanSprite` are per-spot `EditAnywhere` slots and both are
optional. With nothing assigned the screen draws a sky, a ground, a horizon
and a shape, and still says everything it needs to. **Per-spot rather than
global on purpose:** the road up to the Shaded Cave should not look like the
road into town.

## The checker learned three things, one of them humbling

`check-cues.py` (added yesterday for sound slots) now covers `UTexture2D`
too. Pointing it at these two slots broke it three ways in a row, and all
three were found by reintroduction rather than by reasoning:

1. **False positive.** It only searched a header's *paired* `.cpp`, and a
   readout struct is declared on the game instance, filled by a spot and
   drawn by the HUD — which is the entire pattern this presentation layer is
   built on. It searches the module now.
2. **False negative from a bad name.** The field was called `Van`, and this
   module has `Van`, `VanRuns`, `VanLine` and `VanNews` already, so a
   word-boundary search found it "used" no matter what. Renamed `VanImage`
   — the checker's limitation was real, but the immediate cause was my
   field name.
3. **Counting writes as uses.** `T.Backdrop = RoadBackdrop` is a mention, so
   the check proved only *somebody fills this*, which is exactly the half
   that is never the bug. It counts reads now.

**And then it stopped.** After all three fixes, deleting the `DrawTexture`
call still passes — because `if (T.Backdrop)` is a null-guard, and a
null-guard is a genuine read. Telling "guarded" from "actually drawn" needs
a parser and this is a grep.

That limit is now written into the checker's own docstring rather than left
to be discovered. It reliably catches what it claims to catch — a slot
nothing in the module mentions at all, which is what happens when a property
is added and the call site is forgotten — and `TopOutSound` and `FallAnim`
both still fire on it. **A checker that says what it cannot do is worth more
than one that implies it does everything.**

## At the desk

Nothing to place. Drive anywhere and the road is there.

Worth trying on purpose:

1. **Drive to a crag.** Watch the clock run and the van cross.
2. **Mark a travel spot's `DestinationZone` as `Town`** and go again — it
   should say *walking*, show no van, and cost no fuel.
3. **Press any key mid-road.** It should cut straight to the far kerb.
4. **Break the van, then drive.** The breakdown line should appear on the
   road rather than as a toast after it.
5. `RoadBackdrop` and `VanSprite` when you have art. A side-on van silhouette
   and a wide landscape strip is all it wants.

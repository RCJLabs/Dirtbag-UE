# Where you park

*2026-08-24*

`Sim/DirtbagBivy.{h,cpp}`, SAVE v41. `BIVY-1` ported from the 2D source.
The original's own line for it: *"Everybody who lives like this has a map of
five or six of them."*

## Why this one, now

It is a trade across seven axes, and **six of them are systems this port
already had** — rest, psyche, the cold-night sickness term, fuel at the
pump, grime, and a cooldown. Every one was built for something else first.
A spot table is the cheapest possible way to make somebody choose between
them, which is why the 2D game gets so much out of five rows of data.

| | free | psyche | exposure | drive | grime | cooldown |
|---|---|---|---|---|---|---|
| **The Lot** | yes, **and the city knows where it is** | 0 | 1.00 | — | — | — |
| The Upper Trailhead | | +2 | 1.35 | 15 min | | |
| The Ridge Spot | | **+6** | 1.50 | 30 min | | |
| The Truck Stop | | −2 | **0.70** | | **−8** | |
| A Friend's Driveway | | **+8** | **0.35** | | | **4 nights** |

**No spot wins on every axis**, and there is a test that says so — if one
did, the decision would not exist. The ridge is the best night's sleep in
the game and costs a road; the driveway is better than either and is not a
place you live; the truck stop is loud and lit and has showers.

**Two have to be earned**, and the reasons are the good part. The ridge is
*"the pull-off the locals do not put in the guidebook — you had to be told
about this one"*, so it wants you to be a local somewhere. The driveway is
*"somebody said any time, and meant it"*, so it wants somebody who knows
you — rapport, the system that got its floor fixed on Tuesday.

## What makes the free one cost something

Two nights grace. Then a chance per night that climbs to a cap, a $25 fine
that accrues unpaid, and at three of them **the city clamps a boot on the
van** — and a booted van does not go to the crag. Clearing it is all of it
or none: the city does not do instalments.

**The counter resets by going somewhere else**, which is the entire reason
the other four exist. The Lot works, and then it stops working.

## The unit error I nearly shipped

The source's spots carry `fuel: 3` / `6` / `1` — 2D **tank units**. This
port has no tank; it prices fuel per driving hour at the pump (`FuelFor`).
Porting the number rather than the meaning would have been a unit error
wearing a dial's clothes.

The blurbs turned out to state the real answer: the upper trailhead is
*"fifteen minutes up the dirt road"*. So the field is `driveHours` and the
times come from the prose.

## And it broke a test, correctly

`TestBillsLandWeekly` asserted a career's cash after a fortnight equals the
bills exactly. It now doesn't, because the default spot is the Lot, the Lot
is free, and after two nights the city starts writing tickets. That is the
system working — the test now parks up the trailhead, which is what the test
was actually about.

Deliberately not here: **the knock** (`BIVY-2` — two in the morning and
somebody is at the window), which has its own per-spot population and is a
pass of its own. Its `knock` multiplier is **not** in this table, because a
dial nothing reads is a dial that lies. That is the same call this project
made an hour earlier about the house battery, and `check-dials.py` is what
made it both times.

# Sport climbing: the rope, the bolts and the runout

`Discipline::Sport` had existed since Phase 0 and did almost nothing — it
added a mid-route rest stance and was otherwise resolved exactly like a
six-move boulder. This is the difference, and building it turned up a
balance hole nobody had measured because sport was never really used.

## What actually separates the two

**Length.** A boulder is 6–8 moves and decided by power; a pitch is 16–20
and decided by pump — which is why the pump bar, the 2D game's best verb,
matters more here than anywhere else.

**You fall on the rope.** The crash pad is irrelevant above the first bolt,
so the ground-fall penalty has to *stop applying* there. Held wrong, it
follows a roped climber thirty metres up and taxes them for having no foam
under a route nobody would put foam under. In its place: the runout — how
far above your last clip you are, paid out of head, and worth less to a
climber with a good one. Below the first bolt you are still bouldering, and
the pad still counts, which is exactly right.

**Clipping.** Pulling up slack with one hand off a hold costs pump, and more
from a bad stance — the route already describes the stance through
`restQuality`.

## The hole this found

A V7.8 climber, resolved against the pitches the generator makes:

| | onsight | wired |
|---|---|---|
| **before** — sport V6 | 65.3% | — |
| **before** — sport V7 | **0.9%** | — |
| **before** — sport V8 | **0.3%** | — |

Sport routes were essentially unsendable at or above the climber's level,
and had been since Phase 0. Pump accrued for the full nineteen moves with
one guaranteed rest — a boulder with a ledge in it, not a pitch.

Two fixes, both structural rather than tuning:

- **A pitch has stances.** Sport routes now get a rest roughly every four
  moves, which is what lets a climber stay on one for twenty moves at their
  limit. This is a route-generation change and touches no boulder.
- **Beta is worth more on a longer route.** It was a flat half-grade
  regardless of length, which quietly said that learning a twenty-move pitch
  is worth no more than learning a six-move boulder problem. Per-move odds
  compound, so half a grade could not pay for fifteen chances to fall: a
  climber a grade above their level topped out at **4.8% even fully
  rehearsed**. Beta now scales with length — flat to 8 moves (so every
  boulder number in the project is untouched, exactly) and 2.2× by 22.

And one of my own, caught by measuring: `clipPumpCost` started at 9.0
against a `basePumpCost` of 11.0 — a clip nearly as expensive as a whole
move — which took sport's send rate from 65% to **zero at every grade**.
It is 3.0 now.

## After

The projecting curve on a pitch at the climber's limit:

| pitch | onsight | half-wired | wired |
|---|---|---|---|
| V7 (a grade below) | 73% | — | 94% |
| **V8 (at the limit)** | **15%** | 40% | **67%** |
| V9 (a grade above) | 0% | — | 8% |

That is a curve: an onsight is possible and never the expectation, every
burn you learn from pays, and a grade above your level is a season rather
than an afternoon. A pitch is also a bigger ask than a boulder of the same
grade — fifteen chances to fall are not one chance to fall — which is both
true and now measured.

## Still desk work

Belay, clipping animation and lowering are presentation, and
`concepts/DIRTBAG.md` defers rope staging to MVP rung 2. The sim half is
done and testable; none of it can be *seen* until there is a rope on screen.

Also still open: a pitch needs a belayer, and the Lot exists. Nothing yet
requires a partner to be present before you can tie in — that is the first
thing sport should take from the Lot, and it is the cheapest real coupling
left in the design.

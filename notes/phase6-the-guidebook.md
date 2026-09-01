# The guidebook

*2026-08-22. Phase 6.*

`Sim/DirtbagCrag.h` carried three `unwired-ok` notes that all said the same
thing:

> *"the guidebook page's filter; **there is no guidebook screen**"*

`LinesUpTo` and `OpenProjects` were written for a page that did not exist,
and had been sitting there for months being honest about it.

## The hole in the middle of the table

Without a book, you learn about a line by walking up to it. There is no way
to see what is at a crag before touring it, which lines you have done, where
the open projects are, or how many burns are in the thing that keeps
spitting you off.

That last one matters most. `concepts/DIRTBAG.md` §4 lists
**projecting/nemesis tracking** as port-wholesale, and `ProjectMemory` has
tracked attempts, best highpoint, beta and cleanliness *per line* since
Phase 1. It has only ever surfaced as **"attempt 14"** on the wall you
happened to be standing at. The tracking was ported; the seeing was not.

For a game whose entire loop is *pick a line, try it*, and whose fiction is
a guidebook, that was the hole in the middle of the table.

## What it is

**G**, at any spot or any wall. Three pages on 1/2/3:

| page | what it answers | wires |
|---|---|---|
| **everything** | what is here | — |
| **projects** | what could be a first ascent | `OpenProjects` |
| **in reach** | what I can actually climb | `LinesUpTo` |

Each line reads as the book would read it — *"Diesel  V5  ***"*, or
*"project — the arete left of Diesel"* — with **your pencil marks in the
right-hand column**: done and in how many burns, or the burn count and
highpoint, or how much of a project you have cleaned. A line nobody has
touched says nothing, which is how a book works.

The header says **how much of the crag you have got through** — the one
place the game ever states it, and the number a climber actually keeps.

Colour does one job: done is quiet green, unclimbed is gold, everything else
is plain. A page that shouts every line is a page you stop reading.

**Nothing is ever silently swallowed.** A canvas HUD has no scrollbar, so
the page caps at what fits and says *"7 more below the fold"*, and a filter
that hides rows says *"12 not on this page."*

## Two hazards, both real

**The gym would have shown you Roadside.** `EnsureCrag` falls through to
`Crag` at the Gym — correct for its own purposes, wrong here — so pressing G
indoors would have handed you the page for a crag forty minutes away. It
refuses now, and says why rather than doing nothing: *"Plastic. The setter's
tag is the whole of the book here."* Detected by asked-to-open-and-still-shut
rather than by re-testing the venue at the call site, so the rule about where
there is a book lives in one place.

**G would have double-fired.** Every spot and every wall binds it, and the
Lot has four spots within a few metres. Standing between the van and the fire
would have opened and immediately closed the book — which looks exactly like
the key not working. It ignores a second toggle on the same frame.

## And the thing the audit actually found

Building the *in reach* page needed `PeakGradeEver`. It is read in exactly
two places and **written in none.**

- `TimeToThinkAboutIt` reads it — the one opinion this game ever offers about
  stopping, and the trigger for the handover built an hour ago.
- `RetireAndPassItOn` resets it.
- Nothing ever sets it.

So it has been a permanent `0.0`, which makes the *"two grades off your
best"* arm of that function dead code. And the other arm reads
`ConsecutiveInjuries`, which **was also never written.**

`TimeToThinkAboutIt` has never been able to return true, for either reason,
in the entire life of this project.

That is written-and-never-wired at a **sixth** layer and the nastiest kind so
far. The others were dead features — a system with no verb, a phase with no
door. This is a **live function silently answering from constants**, and no
checker here could see it: they prove declarations are reachable, not that
fields are ever assigned.

Both are written at Sleep now. `PeakGradeEver` is a high-water mark of
ability grade — stored rather than derived, because you cannot recompute the
best you ever were from the body that is left — and `ConsecutiveInjuries`
counts on the day an injury *starts*, not every day you are hurt, resetting
after a clean season rather than a clean day. Three injuries in a row is the
signal; one injury lasting three months is not.

## At the desk

1. **Press G at a crag.** The page should be the crag you are at.
2. **1/2/3.** Projects should be the gold ones; in reach should thin the
   list to what you have actually climbed.
3. **Go and fail on something four or five times, then press G.** The burn
   count and highpoint should be sitting there in the right-hand column.
   That number has existed since Phase 1 and you have never seen it
   anywhere but on the wall itself.
4. **Press G in the gym.** It should refuse and say why.
5. **Stand in the middle of the Lot, between spots, and press G.** It should
   open once. If it flickers, the same-frame guard is not working.

## Known, and deliberately left

At the Lot, the book shows whichever crag you were at last, because `Venue`
is the last rock you stood on and the Lot is not a venue. That reads
correctly most of the time — you are planning tomorrow at the crag you were
at today — and now that `Sim/DirtbagZones.h` exists, wiring the page to the
zone instead is the obvious later fix.

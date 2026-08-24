# DECISION — gym ownership is back in scope

*2026-08-24. Evan, on being shown the measured ceiling: "OK how about 1 now"
— option 1 being un-cut it and build it.*

**Status: decided, and deliberately not yet scoped.** The reversal is
recorded here. What it becomes is not, and the reason is written down
below rather than guessed at.

## What is being reversed

`concepts/DIRTBAG.md` §4 defers gym ownership past 1.0, under one
justification shared by nine entries: *"The 2D game took years to accrete
these; the 3D game earns them the same way."*

`DECISION-comps-are-back.md` then **re-affirmed this specific cut** on
2026-08-23, naming gym ownership among the entries the cut list got right:
*"it is right about expeditions, deep-water solo, big wall, the photography
economy and gym ownership, which really are late accretions on a game that
already worked without them."*

That reasoning was sound and it was made without a number. It has one now.

## The evidence that changed it

Full measurement in `notes/the-ceiling-nobody-can-spend.md`. The short form:

- The most expensive thing in the game is **Home Base at $30,000**. Above
  it there is nothing.
- A **hoarder** — which climbs **755 days**, more than any other policy,
  sends as much as anybody, and ends with the **equal-best all-round grade
  in the game** — holds **$60,378**. Twice the ceiling, for a climber who
  is playing well.
- A salaried career holds **$609,045**. Twenty times the ceiling.
- `DreamDials` priced honestly off a savings sweep, and says so. That sweep
  was of the climbing-first wallet, which peaks at **$474**. Nothing in the
  price list was written to absorb $600,000 because nothing produced it
  when the prices were set.

**This is the same shape of argument that brought comps back.** That one
was *the port has one arc and the 2D game has two*. This one is *the port
has a money curve with a ceiling at $30,000 and a wallet that reaches
$600,000*, and the 2D game's answer to that is a business you buy and run.

## What this is NOT yet, and why

**It has not been scoped from the 2D source, and it must not be built until
it is.**

`notes/the-2d-audit.md` exists because the previous audit was run against
**§4's nineteen-item paragraph** — a summary written from memory — and
reported the result as if it were the game. Its own verdict on that:
*"Auditing against the summary and reporting it as the game is the same
error as testing an ordering when the question was a magnitude — the check
passes and tells you nothing."*

The 2D source (`dirtbag_v0957`, `App.tsx` at 44,171 lines, a taxonomy of
122 families and ~450 numbered features) **is not in this repo**. It was
supplied as zips in an earlier session. So everything currently knowable
about gym ownership here is one clause in one table row.

### The one real signal that clause carries

`the-2d-audit.md` lists it under **Coaching & mentoring**, not under any
economy or business family:

> a mentee with milestone beats, a persistent **coaching roster** with real
> training plans, the youth team, head-coach days, **gym ownership**

That is worth taking seriously, because it points somewhere different from
where the money argument points. The money argument says *a sink for
$600,000*. The taxonomy says *the late-career arc where a climber stops
being the one who climbs and becomes the one who makes climbers* — and a
gym is where that happens rather than the point of it.

**Those two readings build different systems.** One is a business with
revenue, staff, rent and a balance sheet. The other is a place you own so
that mentees, a coaching roster and a youth team have somewhere to be, and
the money is the entry fee rather than the mechanic. Guessing between them
from a table row is how this project got the last audit wrong.

## What is needed to scope it

From the 2D source, specifically:

1. **Which taxonomy family and feature numbers** cover it, and what the
   sibling features in that family are. If it is `COACH-n`, it is the
   coaching arc; if it has a family of its own, it is a business.
2. **What `GameState` persists for it** — the 497 persisted fields are the
   honest inventory of what a system actually is. Revenue and staff rosters
   look different from a member count and a set list.
3. **What it costs and what it returns**, so the ceiling argument can be
   checked against the original's own numbers rather than against mine.
4. **How it ends.** `Work Is A Craft` already ports *gym sets that stay up
   and get climbed by other people*, which is adjacent and built; whether
   ownership is the top of that ladder or a separate thing matters.

## What is already true here, and is not nothing

Phase 12 shipped **setter as a craft skill, employer standing per building,
and gym sets that stay up and get climbed by other people**. Whatever gym
ownership turns out to be, it lands on top of a system that already models
setting, standing with a building, and work identity. That is a materially
better starting point than the cut list assumed in either direction.

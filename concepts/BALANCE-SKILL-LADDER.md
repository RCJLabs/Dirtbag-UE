# The skill ladder, recalibrated (2026-08-17)

Logged because it changes what every skill number in the game *means*, and
future-me will want the reasoning rather than the diff.

## What was wrong

The resolver read a skill as `skill / 100 * 18` grades. That made the
default 50-stat climber a **V9 climber**, so a V0–V7 gym board offered them
nothing: measured send rates were 93% on V6 and 74% on V7 for a *starting*
45-stat climber, and 100% across the whole board after a nominal season.
Every grade read the same, which quietly undermines the entire premise —
"the sim decides" is meaningless if the sim always says yes.

Two follow-on bugs fell out of the same root:

- `DirtbagDay` duplicated the same formula to decide whether an attempt was
  hard enough to train anything. With ability inflated ~4 grades, the
  challenge term was permanently zero: fingers moved **0.4 points a month**
  under 42 burns a week.
- Endurance skipped that gate entirely (raw mileage), so it trained ~50×
  faster than every other skill. A month of climbing added 22 points of
  endurance and nothing else.

None of this was visible from the tests, which asserted *relative* truths
(strong beats weak, fit out-lasts unfit) that stayed true on an inflated
scale. It took simulating a month and reading the numbers.

## The mapping now

`SkillToGrade(skill) = skill/100 * skillGradeSpan + skillGradeFloor`, span
14, floor −2, living in `SessionDials` and used by **both** the resolver and
the trainer — one source, because two copies drift and the drift is silent.

| skill | ability | reads as |
|---|---|---|
| 30 | V2.2 | first season |
| 50 | V5.0 | solid intermediate |
| 65 | V7.1 | strong local |
| 80 | V9.2 | very strong |
| 100 | V12 | professional |

0–100 spans a whole climbing life, and both ladders stay open-ended at the
top, so V13+ remains the domain of the exceptional.

What that produces for a V5 climber, single attempt: **V4 85%** (warmup),
**V5 23%** (a few burns), **V6 1%** (a project), **V7 0%** (next year).
Pinned by `TestGradesResist` so it can never silently re-flatten.

Training was retuned to match: a committed week at or above your limit is
worth roughly a third of a grade at mid-career, with diminishing returns
(`trainingCeiling`) making the last points of a skill a grind. Endurance
still trains on mileage but deliberately below the targeted skills —
volume supports a career, it doesn't replace trying hard.

## Open, deliberately not decided here

**The economy doesn't bite yet.** One shift a day nets ~+$44 and cash grows
without bound (day 28: $1312); never working goes bankrupt around day 21.
So there's a cliff, not a squeeze — and climbing itself is free, which no
gym has ever been. The obvious candidate is a day pass charged on pull-on,
plus a wage/bills pass.

Left alone on purpose: the money loop is **Phase 3's** gate, explicitly
measured against the 2D game's reference balance, and that reference lives
with Evan, not in this repo. Guessing at it now would be improvising over
three years of tested truth.

**Emergent and probably correct:** four burns a day, seven days a week pins
skin at the floor permanently (regen 1.5/night loses to ~3/day of use), so
the loop already argues for rest days without a rest-day mechanic. Worth
watching in playtest rather than "fixing".

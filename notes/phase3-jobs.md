# Work, and the trap that is not one yet

## The board

Ten authored gigs, three on the board a day, drawn without repeats and with
pay wobbling ±15% so the same job is worth having some weeks and not
others. Deterministic per world and day, on its own rng stream.

The coupling worth having: **some gigs need the van** — hauling firewood,
moving furniture, trail work, shooting guidebook photos. So a breakdown
costs you the repair *and* the work that would have paid for it, which is
the most dirtbag sentence in the whole economy. A test pins it.

Odd-job pay goes to debt first, like any other money.

## The salaried job, measured — and it is not a trap

The design intent: a job that solves money completely and costs you the
thing money was for. Nine to five, five days, $520 a week.

The first half works exactly as intended. A salaried year ends with
**$19,669** against the dirtbag's **$145**. Money simply stops being a
question.

The second half does not work at all:

| | climbed | burns | sends | grade | cash |
|---|---|---|---|---|---|
| dirtbag (crag-1) | 109 | 528 | 1 | V5.2 | $145 |
| salaried (crag-1) | 109 | 534 | 1 | V5.2 | $19,669 |
| dirtbag (crag-3) | 113 | 527 | 11 | V5.5 | $147 |
| salaried (crag-3) | 110 | 530 | 6 | V5.4 | $19,709 |

**The salaried climber climbs the same number of days.** The reason is
specific and I did not see it coming: work finishes at 17:00, and Roadside
Crag is east-facing, so its window is *the evening*. A nine-to-five and an
east-facing crag do not conflict. The job takes the hours the rock was no
good in anyway.

Raising the energy cost does not fix it — 45 → 65 → 80 gives 3, 4, 4 sends
on one seed and 6, 2, 2 on another. That is noise, not a mechanic, so the
dial is back at 45 rather than tuned until a number looked right.

## What the trap actually needs

It is not more hours or more tiredness. The cost of a job is **that you
cannot chase conditions**, and that only bites when conditions move:

1. **Aspect.** A west-facing crag peaks in the morning and a nine-to-five
   would gut it. The trap is real there and absent here — which is a good
   argument that crag choice should matter, and an argument against the
   single-crag world rather than against the mechanic.
2. **Seasons.** One `baseTempF` covers the whole year, so windows sit in
   roughly the same part of the day forever. A summer that pushes everything
   to dawn would make a nine-to-five impossible for months.
3. **Spontaneity.** The good day is the one you did not plan for. Nothing
   in the game yet rewards being able to drop everything, because nothing
   yet stops you.

The mechanic is built, tested, and honest. It is not yet a trap, and
pretending otherwise by inflating a dial would be worse than saying so.
Seasons are the cheapest of the three fixes and the one I would do next.

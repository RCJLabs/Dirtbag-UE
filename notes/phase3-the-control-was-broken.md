# The Phase 3 control was broken for five measurements

**2026-08-21, evening.** Six mechanics changed today, so the season probe was
re-run from scratch. The re-run found something older and more important than
anything today's changes did.

## The `kept` control could never buy shoes

`kept` is the control the whole Phase 3 gate rests on: *a player with no
bills, free food and free fuel, who never works a day.* The difference
between their season and a real one is supposed to be, exactly, the price of
being alive.

It was not. Zeroing the bills also removed every reason to work, so the kept
player **earned nothing, ended every year on about $2, and could never
replace their shoes**:

| | end shoe wear | cash low | cash end |
|---|---|---|---|
| greedy (works 19.5% of days) | 0.30 | $48 | $175 |
| **kept** | **0.89** | **$2** | **$2** |

Dead rubber is worth roughly **3.7 sends against 1.0** (`notes/shoes.md`).
That single omission accounts for the whole of the anomaly the first Phase 3
note recorded and could not explain — *"and **fewer** sends than the player
paying rent"*.

**A control that removes the costs and the income is not a control for "does
money pressure climbing". It is a control for being broke.** The kept player
now starts with a $10,000 float, which is what "money is not a question" was
always supposed to mean.

## With an honest control, the answer is the same

Twelve seeds, a full year each. Shoe wear now matches (0.29 vs 0.30), so the
confound is gone.

| | sends | climbing days | burns | allround | days worked |
|---|---|---|---|---|---|
| greedy | **3.50** | 181 | 517 | 5.51 | 19.5% |
| kept | **2.92** | **196** | 528 | 5.53 | 0% |

Per seed: kept wins 1, **ties 7**, loses 4.

So the conclusion holds, and now for a clean reason rather than a confounded
one: **seventy days of not working buys fifteen climbing days and eleven
burns, and converts them into nothing.** Skill lands within 0.02 of a grade
of the player who worked all year.

The honest reading of the sends column is *no detectable difference* — at
this spread (0–8 across twelve seeds, seven of them exact ties) a mean gap of
0.58 is noise, and it points the wrong way for the gate regardless.

## Why this matters more than the number

The previous five measurements reached the right conclusion through a broken
control. **A right answer from a wrong method is not evidence**, and the
paradox it produced — the kept player sending *less* — was visible in the
first note, recorded, and never chased. It should have been. An anomaly you
cannot explain is the measurement telling you the instrument is wrong.

What it changes for the decision Evan is sitting on: nothing, and that is
worth saying plainly. **Option A (stop skin being the sole cap) was already
refuted on its own measurement, and the gate still does not close on a sound
control.** Restating the gate remains the live option. The evidence for it is
now better than it was this morning.

## Two probe/game divergences fixed on the way

The probe called `NameFirstAscent`, not `ClaimFirstAscent` — **the same
half-a-verb bug the engine had**, so every simulated first ascent was worth
no opinion to any faction, and standing is what earns a sponsor. And
`ReviewSeason` ran nowhere, in the probe or the engine, so no career had ever
been reviewed. Both now match the game.

Still divergent, and recorded rather than invented: the probe has no
"ask for beta" or "sit at the fire" policy at all, so `ShareBeta` and
`PsycheFrom` — both wired into the game today — are not modelled. Giving the
simulated player a social policy is a design choice about what that player
does with an evening, not a bug to fix quietly.

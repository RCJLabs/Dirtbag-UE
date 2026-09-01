# The gym, pass two

*2026-08-25*

`Sim/DirtbagGym.{h,cpp}` extended, `Sim/DirtbagGymTown.{h,cpp}` new, SAVE
v42. `GYM-3`, `GYM-4`, `GYM-6` and `GYM-12` ported from the 2D source, plus
`GYM-1`'s own phase 3.

`notes/the-gym-pass-one.md` ended with a claim and an IOU: *"pass one
absorbs about $41,000 — $25,000 for the building and roughly $16,000 of
levers — against a hoarder's $60,378. Most of the absorption lives in the
parts not yet ported. The argument holds; it needs pass two to fully
land."* This is that.

## What pass one got wrong about itself

Every lever in pass one pulled against **a static number**. Set the price,
pick the mix, buy the mats, and the membership drifted toward a target that
had already decided what it was going to be. There was no way to have a bad
month you did not cause, and no reason to look at the place again once the
levers were where you wanted them.

Four systems fix that, and they are four different shapes on purpose:

| | what it is | what it moves |
|---|---|---|
| `GYM-4` | the other two gyms in town | a **multiplier** on the target |
| `GYM-6` | the year | another multiplier, seasonal |
| `GYM-6` | incidents | a bill or a consequence, with a clock |
| `GYM-3` | the staff, as people | the boost you already had, times a person |
| `GYM-12` | wings | the base, and **who** comes rather than how many |

## They move the multiplier, you move the base

That split is the whole of `GYM-4`, and the source records paying to learn
it. Its first cut scored your price tier, mix, equipment and campaign into a
"pull" and took your share of the town from it — **double-counting the
player's own choices**, because every one of those is already in the target.

So: your levers do exactly what they always did, and pressure answers one
question — what are the other two rooms up to this month. The single
exception is a live campaign, which blunts them, because that is what
marketing is *for* and it gives a rival's good month a counter-move that
costs real money rather than a shrug.

Rival strength is a pure function of the day and your gym's name. No stored
state, no migration. The name is in there because without it the town's
whole history is one fixed script identical in every save, and the source
measured that script as sitting slightly against the player: 40-year windows
averaged 0.9837 and the best available was 1.0003, so **no playthrough could
ever be dealt a kind town.**

## The one deliberate deviation: the season

The source's four seasonal pulls — `{-0.04, +0.10, -0.14, +0.08}` for
spring, summer, fall, winter — are balanced against **equal calendar
quarters**, and their invariant is that they sum to zero, so the year is a
rhythm and not a tax.

This port does not have equal quarters. `SeasonName` in `DirtbagConditions`
names the season off the temperature, which measures out at:

| | spring | summer | autumn | winter |
|---|---|---|---|---|
| days a year | 61 | 121 | 61 | 122 |

Ported as written against those lengths the four pulls average **+0.0298** —
a silent 3% permanent bonus to every gym in the game forever. That is
exactly the bug `GYM-4`'s own comment records finding in the other
direction, arriving through the back door.

So they are re-centred on this port's calendar: `{-0.07, +0.07, -0.17,
+0.05}`, which measures at |mean| < 0.0002 over a year and keeps the shape
and the argument intact. `TestTheYearIsARhythmAndNotATax` asserts the sum
directly, so it cannot drift back.

The argument, unchanged, is the good part: the obvious real-world answer is
wrong in this world. **The slump is autumn** — send season, when the whole
town is at the crag — and the rush is summer and winter, the two seasons
this game's own climate model calls off.

## The floor never arrives any more

`TestTheFloorIsAMeterAndNotASwitch` used to assert that the membership
reaches its target inside a couple of months. It does not any more, and that
is the point: a rival move lands every nine days and the floor closes 15% of
the gap a day, so **the membership tracks rather than settles.** Measured
over 800 days the target moves by up to 9 members in one step and the floor
sits up to 14 behind it.

The old test now runs with the town held still — that is what the clamp
dials are for — and `TestTheFloorIsAlwaysChasing` asserts the other half.

## What it absorbs

The ceiling measurement, finished. One gym, fully built:

| | |
|---|---|
| the building | $25,000 |
| the equipment ladder, both rungs | $8,000 |
| both hires | $5,500 |
| all five wings | $22,600 |
| **total capital** | **$61,100** |

Against `notes/the-ceiling-nobody-can-spend.md`'s hoarder, who ends thirty
years holding **$60,378**. Pass one could absorb $38,500 of that. Pass two
covers it, with the ongoing bills — wages, upkeep, and about **367 incident
answers over thirty years of ownership** — on top.

## The measurement that surprised me

`gym=run` in the probe buys the building, staffs it from the top of the
shortlist, builds every wing in table order, grants every raise, pays every
incident bill it can afford, and hands over the keys. Six seeds, thirty
years:

| | pass one (`gym=1`) | pass two (`gym=run`) |
|---|---|---|
| foreclosures | 0.2 | **13.2** |
| highest wage reached | — | **$2,232 a day** |
| raises granted | — | 514 |
| wings built | — | 70.8 |
| incidents answered | — | 366.8 |

**Always saying yes bankrupts the gym in about two years**, thirteen times
over. The raise compounds — 20% of the current wage, every forty days,
forever — and nothing about the target compounds with it. A desk staffer
returns roughly `5 × quality × rate` a day, so at standard pricing they are
worth about $70; base wage $25 passes that after six raises, which is eight
months. **The honest life of a hire is about a year**: grant while it is
cheap, refuse when it is not, refuse again, and hire somebody new.

That is `GYM-3` working exactly as it claims to — *"they don't work here
forever for the same money"* — and the probe's naive policy is the proof
that the raise is a decision rather than a formality. It is worth saying out
loud that the game never tells you where the crossover is; you find it by
watching the balance.

**One thing the measurement cannot say.** `gym=run` also shows the climber
getting substantially better — technique 74.5 → 100, head 60.4 → 83.2. That
is **not the gym**. Draining the wallet stops the hoarder policy from ever
reaching its savings target, so it works 72% of its days instead of 18%, and
a career that works differently climbs differently. A no-gym hoarder with
the target set to $200,000 reproduces the same days, burns and power exactly.
It is a policy artefact, and it is recorded here so nobody quotes it later
as a benefit of owning a gym.

## What the probe was not building

Finding the above needed the probe, and **the probe had not compiled since
the turnout commit.** `SpendDayWith` gained a required `smell` argument and
`Sim/tools/season.cpp` was never updated, so every measurement claim made
between that commit and this one could not have been re-run — and preflight
stayed green throughout, because `check-parity.py` reads the probe's
*source* and `never-happened.py` runs a binary that was already on disk.

The tool's own coverage is the bug, for the fourth time. `tools/preflight.sh`
now builds `build/season` as one of its thirteen — fourteen — checks.

And `never-happened.py`'s battery had no gym configuration in it at all,
which would have reported the entire business dead. Both passes are in it
now. The one counter still not moving is `gymquit`, and the reason is that
the probe always grants: nobody has ever been refused twice.

## Cut on purpose

- **`GYM-2` and `GYM-5`** — the members as named people with arcs, the floor
  walk, comp night. That is the narrative layer and a big text table; it is
  its own pass.
- **`GYM-8`, `GYM-9`, `GYM-10`** — the youth team, hosting a circuit round,
  the league you run. Each reaches into a system this port has but has not
  connected to the gym.
- The wing table's `youth` and `bid` fields, which feed exactly those two.
  A dial nothing reads is a dial that lies; they come back with the systems
  that read them.

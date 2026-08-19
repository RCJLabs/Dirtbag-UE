# Does the money pressure the climbing? (Phase 3 gate)

Measured 2026-08-19 with the season probe (`Sim/tools/season.cpp`), 365 days,
five world seeds, rest-until-skin 3.0. Every number below is one call away
from the functions the game calls.

## The answer: no. Not at the odd-job scale.

The probe cannot answer the question by playing normally, because every
policy in it works whenever the money runs low — a thermostat can't tell you
how cold it is outside. So the measurement is a control: a `kept` player with
no bills, free food and free fuel, who therefore never works a day. The
difference between their season and a real one is exactly the price of being
alive.

| seed | policy | days climbed | burns | sends | days worked |
|---|---|---|---|---|---|
| crag-1 | kept | 133 | 524 | 0 | 0 |
| crag-1 | careful | 135 | 522 | 1 | 73 |
| crag-2 | kept | 132 | 502 | 0 | 0 |
| crag-2 | careful | 133 | 506 | 4 | 69 |
| crag-3 | kept | 127 | 516 | 2 | 0 |
| crag-3 | careful | 129 | 513 | 5 | 73 |
| crag-4 | kept | 133 | 520 | 1 | 0 |
| crag-4 | careful | 135 | 527 | 5 | 69 |
| crag-5 | kept | 136 | 528 | 0 | 0 |
| crag-5 | careful | 136 | 528 | 2 | 69 |

Seventy days of work a year costs nothing. Not a climbing day, not a burn,
and not a send — the paying player sends *more* on four of five seeds, which
is noise, but noise is the point: the effect is smaller than the noise.

Nobody went broke or hungry in any of the fifteen seasons run.

## Why

Three reasons, in order of how much they matter.

**1. The day has room for both.** An odd job is four to six hours starting at
seven. The prime window is one hour, usually later. A shift costs you the
morning, and the morning was never the climbing. Only the salaried job takes
the day, and it shows: 478 burns against 522, and 106 windows a year arrived
at after they had gone.

**2. Skin caps the year, not time.** A player resting to skin 3.0 climbs about
133 days out of the 208 that have a window at all. There are 157 washed-out
days and 75 rest days a year to put work in. The calendar is not scarce.

**3. There is nothing to want.** This is the real one. Across a full season
the only discretionary spending in the game is shoes — $275 on the seed above,
against $6,601 earned. The player ends the year with $133 in the bank and
nothing to spend it on. Money cannot pressure climbing because money does not
*buy* climbing.

## What was fixed along the way

- **Seasonal daylight.** `firstLight`/`lastLight` were fixed at 6-to-20 all
  year, so a nine-to-five always left three hours of evening. They now swing
  with the year around solar noon: 16.5 hours midsummer, 7.9 midwinter, with
  the solstice leading the warmest day by thirty. In midwinter the light is
  gone at 16:26 — before you clock off. `TestTheLightGoesInWinter` holds it.
- **Fuel.** Driving wore the van but cost nothing. At $9 an hour — a van at
  eighteen to the gallon, which is close to the real arithmetic — a season
  spends $918-$1,224 getting to the crag. It is the only cost in the game
  that goes *up* the more you climb, and it moved days-worked from 15% to 19%.
- **The probe drove to locked gates.** It drove out on every day with a
  window, including days the crag was shut and days it had already decided to
  rest — a hundred pointless hours a year, free until fuel had a price.
- **Cleaning charged for every brush.** `ScrubbedALine` fired each time,
  which put a projecting player at -0.70 with the stewards having never once
  taken the money work. Only virgin rock counts now; going over an
  established line before you pull on is maintenance and nobody has an
  opinion about it.

## The faction axis, measured

With the scrubbing fix in, the greedy/careful split is clean but empty:

| policy | stewards | closures | days shut out | cash at year end |
|---|---|---|---|---|
| greedy | -0.23 … -0.95 | 0-12 | 0-99 | $132-$196 |
| careful | -0.05 … -0.23 | 0 | 0 | $133-$196 |

The consequence works — take the guidebook money and the gate closes for a
third of the year. But the temptation does not: greedy ends the year with the
same cash as careful. Turning the money work down is free, so it is not a
choice. Same root cause as above — there is nothing the extra money buys.

## What this says to do next

Not a dial pass. The gate fails for a structural reason and tuning bills or
gig pay will not move it: at $85 a week the player is already within $200 of
breaking even, and doubling it just means working thirty more of the 232
spare days.

The missing mechanic is **things worth money that make you a better
climber** — a rope, a rack, a pad, a gym membership through the winter, the
tank of gas to a better crag. Then a shift is a choice made against a
climbing day rather than a chore done on a rest day, the guidebook money is
worth what it costs you, and the same probe re-run answers the same question
differently. Until then, Phase 3's gate is honestly unmet.

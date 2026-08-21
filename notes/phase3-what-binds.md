# What binds, if not skin

**2026-08-21.** Five measurements had ended at the same wall — *skin caps the
year, so nothing money buys can add climbing to a season* — and left two
options on the table: **stop skin being the sole cap**, or **restate the
gate**. Both were guesses about a thing nobody had measured.

This is the measurement. Method: a probe knob for `skinRegenPerNight`
(shipped 1.5, so nine points of skin is six nights), swept 1.5 → 9.0 across
12 seeds × 2 policies, 365-day seasons. Nothing shipped changed.

## 1. Lifting skin does not add climbing. It adds injuries.

Greedy policy, 12 seeds, mean:

| regen | skin/yr | burns | vs linear | days out | injuries | days hurt | days+hurt |
|---|---|---|---|---|---|---|---|
| **1.5** (shipped) | 548 | 513 | 100% | **181** | **0.0** | **0** | 181 |
| 2.0 | 730 | 630 | 92% | 173 | 0.5 | 9 | 183 |
| 2.5 | 912 | 672 | 79% | 159 | 1.4 | 25 | 185 |
| 3.0 | 1095 | 725 | 71% | 152 | 1.8 | 38 | 189 |
| 4.0 | 1460 | 753 | 55% | 139 | 1.9 | 42 | 181 |
| 9.0 | 3285 | 790 | 26% | **121** | **4.2** | **90** | 211 |

Days at the crag go **down**, 181 → 121, as skin goes up. A six-fold increase
in the skin budget buys **54% more burns** and costs **four injuries and 90
days hurt a year**.

`days + hurt` is flat at 181–211 the whole way. The season has a fixed number
of usable days and the only thing raising skin changes is what fills them.

**Skin was never the wall. It is the guardrail in front of the wall.** The
Phase 4 body measurement already said an outdoors-only climber cannot
overtrain — 0 injuries in twelve seasons — and read that as the weather
protecting them. It is not only the weather. It is skin: shot skin is what
stops you before your tendons do. Take it away and the load system, which has
been dormant since it was built, becomes the binding constraint immediately.

Behind injury is the weather, and it is close: burns saturate at ~790 against
a 208-day window supply.

## 2. Option A is dead. The gate does not close at any skin setting.

The `kept` control — no bills, no food, no fuel, never works a day — against
`greedy`, per seed:

| regen | kept − greedy days | kept − greedy sends | kept out-sends working |
|---|---|---|---|
| 1.5 | +14.5 | −4.1 | **0 of 12 seeds** |
| 2.0 | +7.7 | −0.8 | 2 of 12 |
| 2.5 | +13.5 | −4.8 | 1 of 12 |
| 3.0 | −0.3 | −5.2 | 1 of 12 |
| 4.0 | −10.2 | −5.9 | 1 of 12 |
| 9.0 | −12.5 | +1.2 | 3 of 12 |

**8 of 72 seed-pairs.** Deleting the entire cost of being alive does not buy
climbing at *any* skin level — including the one where skin cannot bind at
all. The working player usually sends *more*.

So "stop skin being the sole cap" would not close Phase 3's gate. It was the
better-sounding of the two options and it is now measured and refuted.
**The remaining honest move is to restate the gate.**

A rest-scheduling hypothesis was tested and refused too: a kept player who
holds out for skin (rest-until 0/2/4/6) climbs 196 → 83 days for 530 → 493
burns. **Burns are near-conserved.** A season's burns are set by skin supply
and almost nothing else; resting redistributes them into fewer, bigger days.

## 3. Two real defects the sweep exposed

**`head` gains exactly 0.00. In every run, at every setting.**

`TrainingWeights` in `DirtbagDay.cpp` carries power, fingers and technique,
with endurance handled separately. Head is not in the training model at all —
and it is *read* in two places in the resolver (`effective` and `nerve`), by
the sport runout and ground-fall code, and by the age system, which says head
is one of two skills that never decline. A player's head is frozen at
whatever they were born with, for life.

Seventh thing written and never wired, after `KitDay`, the body roll,
`FactionDay`, the legacy save path, ethics discovery and `EthicsNews`. This
one is a whole skill axis: read everywhere, written nowhere. **Not fixed here
— what trains head is a design question** (the 2D game's answer is falling
and committing: runouts, highballs, the move you can't reverse), and it wants
a milestone rather than a dial.

**The probe has been reading the wrong skill.** `grade` is
`SkillToGrade(power)`, and power is the *slowest*-growing skill: +2.4 in a
year where fingers gain +7.6. Read off power, a season looks worth a third of
a grade; across all five it is worth about half. Every "grade after a season"
number in this project's notes is that low. An `allround` column now reports
the five-skill mean; `grade` is left alone so old notes stay comparable to
their own numbers.

One year of climbing from 50.0 in all five, 8 seeds:

| regen | policy | burns | power | fingers | technique | endurance | head |
|---|---|---|---|---|---|---|---|
| 1.5 | greedy | 520 | +2.35 | +7.60 | +3.29 | +2.21 | **+0.00** |
| 1.5 | kitted | 519 | +2.99 | +8.06 | +4.90 | +2.30 | **+0.00** |
| 9.0 | greedy | 794 | +5.77 | +11.69 | +7.31 | +4.06 | **+0.00** |
| 9.0 | kitted | 989 | +4.71 | +12.69 | +10.21 | +3.96 | **+0.00** |

Progression itself is fine and responsive — double the burns and you roughly
double the gain. That was worth confirming, because it was the other thing
the flat send counts could have meant.

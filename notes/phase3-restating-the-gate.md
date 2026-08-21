# Restating the Phase 3 gate

**2026-08-21.** The original gate: *"the money loop pressures the climbing
loop the way the 2D game's does, measured against the 2D reference balance."*

Five mechanics were measured against it. All five failed.

| mechanic | measured result |
|---|---|
| bills, food, rent | 70 days of work a year cost nothing; sends unchanged |
| fuel | $918–$1,224 a season; sends unchanged |
| the kit | buying everything is neutral; only the pad moves anything |
| physio | money for time; sends unchanged |
| sponsorship | a title deal pays you out of the workforce entirely (21% of days worked → 0%, $39,040 over five years) and takes ~27 good days a year; **sends exactly unchanged** |

And a sixth, today, on a control that could actually afford shoes: deleting
the cost of being alive outright buys **fifteen climbing days and no sends**.

## Why it could never be met

A season's burn count is not set by money and not by time. It is set by
**skin supply**, and behind skin by **injury**, and behind injury by **the
weather**:

- 208 days a year have a window at all. A player climbs about 180 of them.
- Total burns per season sit near 520 and barely move. Six times the skin
  budget buys 54% more burns, and costs four injuries and 90 days hurt
  (`notes/phase3-what-binds.md`).
- Seventy days of work fit entirely inside the days that were never
  climbable anyway. That is why work is free, and why buying the days back
  buys nothing.

So the original gate asked a money system to move a number that money does
not touch. **It is not a hard gate, it is an unmeetable one**, and holding a
mechanic to it produces a failed measurement and no information about the
mechanic.

## What money *does* do, measured

Two purchases move a season, and they are the same shape:

- **Shoes.** Never replacing them: **3.7 sends → 1.0**, same burns, same
  days. Wins 7 seeds, ties 3, loses 0. Costs about $9 of the year's cash.
- **The second crash pad.** **+37% sends**, and it spends no skin.

Both are *kit that raises the value of skin you were going to spend anyway*,
rather than kit that lets you spend more of it. That is the whole category,
and it was identified in `notes/phase3-kit.md` months before it had a gate to
belong to.

**So money buys quality, not quantity.** You cannot buy more burns. You can
buy burns that count.

## The restated gate

> **Money decides how good your burns are, and deciding is something the
> player actually does.**

Three criteria, each measurable rather than felt:

**1. Neglecting your kit visibly costs you climbing.** — **Passes** on
shoes, by a wide margin, across ten seeds with no losses.

**2. At least one purchase that matters is affordable but not automatic.** —
**Passes.** Corrected within the hour, and the correction is the more useful
half.

It first measured as a fail: a whole year of the kit-buying policy spends
$235 and never reaches the pad's $260. But that policy — like every other one
in the probe — works **only when nearly broke**, `cash < 120`. That is a
thermostat, and a thermostat never saves up. The pad was not unreachable; **no
simulated player had ever tried to buy it.**

The same shape of mistake the first Phase 3 note named and did not finish
chasing: *"every policy in it works whenever cash runs low; a thermostat
cannot tell you how cold it is outside."* That was written about the *income*
side. The *spending* side had it too, and nobody looked.

A `saver` policy that works on any day it is short of the pad, twelve seeds:

| | sends (median / trimmed mean) | days out | days worked | head |
|---|---|---|---|---|
| greedy | 4.5 / 3.2 | 181 | 19.5% | +1.85 |
| saver | **5.0 / 4.3** | **168** | **21.1%** | **+0.01** |

Wins 7, ties 2, loses 3. So the pad costs **six extra working days a year and
thirteen climbing days**, and returns about a send. That is a decision with a
price, which is what the criterion asks for.

*(The untrimmed mean is 5.8 against 3.5, inflated by one seed that returned
23. The median and trimmed mean are the honest numbers and are quoted first.)*

Shoes remain at the other end — cheap enough to be reflexive rather than
chosen — but one live decision is what the criterion requires.

**3. The trade is legible while you are choosing.** — **Partly.** The shop
now says what dead rubber is costing you in grades. The pad's cost is not
surfaced anywhere, and it now has a real one: pads buy sends and cost head,
because head only trains on exposure you have not padded away
(`notes/phase4-head.md`).

## The real problem the same run found

The pad's price in head is not a cost. It is an **erasure**.

`padsThatMatter = 2`, so a second pad puts padding at exactly 1.0, which puts
`ExposureAt` at exactly 0, which puts head training at **+0.00 — in eleven of
twelve seeds**. And head never declines, so it is not "slower progress", it is
*no further head for the rest of that career*.

So the trade reads: **about one send a year, in exchange for every point of
head you will ever have.** That is a switch, not a decision. Nobody weighing
it would find it interesting; they would find out afterwards.

The cave is the designed escape hatch — above the first bolt the runout takes
over and pads stop being the question — and it is not placed in the level yet,
and a boulderer may never walk up there anyway.

**Capping padding below 1.0** so that even a well-padded highball keeps a
trickle of exposure would make the trade proportionate. That is a design call
about whether foam ever fully removes fear, not a dial pass, and it is not
made here.

## What this makes the next work

Criterion 2 is the whole of it, and it is a content decision rather than a
dial pass: **the interesting purchase has to land inside a season's reach,
and be worth giving something up for.** Options, none chosen here — Evan's
call:

- Bring the pad within reach (price, or income, or a second-hand one at the
  Lot) so the choice is *pad or a month of the gym* rather than *pad or
  nothing*.
- Give shoes a real decision instead of a reflex: resole cheap and slightly
  worse, or new and dear.
- Make the pad's head cost visible, so criterion 3 closes with it.

## The honest note about method

The original gate was not wrong to write down in 2026. It came from the 2D
game, where it is true, and the only way to find out it did not survive the
port was to measure it — six times, as it turned out, because the sixth
measurement is the one that found the control had been broken all along
(`notes/phase3-the-control-was-broken.md`).

Restating a gate is not the same as lowering it. The new one fails today,
on criterion 2, on a measurement rather than a feeling. That is what a gate
is for.

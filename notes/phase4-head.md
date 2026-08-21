# Head: the skill that was read everywhere and written nowhere

**2026-08-21.** The skin sweep turned up a `head` column that read **+0.00 in
every run at every setting**. Not "a little" — exactly zero, always.

`TrainingWeights` in `DirtbagDay.cpp` carries power, fingers and technique,
with endurance handled separately. Head was never in the training model. It
was, meanwhile, *read* in four places:

- the resolver, twice — `effective += (head − 50)/100`, and `nerve`, which
  halves the fear penalty for a bold climber
- the sport runout and ground-fall code
- the age model, which names head one of the two skills that never decline

So a player's head was frozen at whatever they were born with, for life, and
the age model's "never declines" was true in the emptiest possible way.

Seventh thing written and never wired, after `KitDay`, the body roll,
`FactionDay`, the legacy save path, ethics discovery and `EthicsNews`. The
first that is a whole skill axis.

## What trains it

Not invented here. `concepts/DIRTBAG.md` already answered it in the systems
table: **"Psyche / head — fall-commitment beats above gear."** Head is
trained by committing where it costs something.

The sim already priced exactly that quantity — inside the resolver, as a
local:

```
if (OnTheRope(route, index))   runoutGradePenalty * RunoutAt(route, index)
else                           noPadGradePenalty * (1 - padding) * exposed
```

That is *how bold this move is*, in grade units. It was computed and thrown
away every move. It is now `ExposureAt(route, index, padding)` in
`DirtbagSession`, called by the resolver to price a scary move and by the day
loop to train head off it — one source, because two copies of "how bold was
that" drift the same way two copies of `SkillToGrade` would. The golden
vectors are untouched by the extraction, which is what says it was faithful.

Head then gains on **the boldest move you actually committed to on that
burn**, and three things about that are deliberate:

1. **Peak, not sum.** The boldest move teaches you; twenty moves along a
   well-padded traverse teach nothing.
2. **Falling counts.** It reads the highpoint reached, not whether it went. A
   fall from above the bolt teaches the lesson at least as well as sticking
   the move did.
3. **It is outside the challenge/engagement gate the other four sit behind.**
   Head is the one skill that does not train on trying hard. A scary move on
   an easy line teaches it; a desperate move under two pads does not.

It cannot be farmed. Exposure is zero on move one, zero under full pads, and
zero below the first bolt — the only way to earn it is to be high on
something you could get hurt on.

## The measurement, and the first time kit has cost anything

A year, greedy, 8 seeds, varying only the pads you own:

| pads | head gain | sends | burns | fingers |
|---|---|---|---|---|
| **0** | **+3.76** | 1.6 | 517 | +7.86 |
| 1 (shipped start) | +1.78 | 3.0 | 520 | +7.58 |
| **2** | **+0.00** | 5.5 | 517 | +6.86 |

**Pads buy sends and cost head.** Until now the second pad was a pure +37%
upgrade with no downside — the one purchase measured to matter, and it
matters for free. It now has a price, and the price is the thing that makes
you bold. That is the trade a real boulderer makes standing at the shop, and
it is the first place in this game where money buys something *and* takes
something.

`headExposureRate = 0.20` was sized against that table, not chosen by feel.

## The escape hatch, which is the cave

Two pads means padding 1.0 means exposure identically zero — a fully padded
boulderer can never train head again, and head never declines, so it would be
frozen for the rest of that career.

Except that pads are not the question on a rope. Above the first bolt the
crash-pad term stops applying entirely and the runout takes over, so a safe
boulderer who walks up to the Shaded Cave starts training head immediately.
**The cave is where you get your head back.** That falls out of the model
rather than being arranged, and it is pinned by a test so it cannot quietly
stop being true.

## Verified by reintroduction

- Deleting the head gain: 2 checks fail.
- Ignoring pads in `ExposureAt`: does not compile (`-Werror`, unused
  parameter) — a weaker catch, so the sign-flipped version was tried too, and
  that fails 6 checks across three unrelated tests.

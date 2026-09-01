# What money should buy: measured

Built because `notes/phase3-economy.md` ended with a named cause rather than a
dial: the Phase 3 gate failed because **there was nothing to want**. A whole
season's discretionary spending was shoes — $275 against $6,601 earned — and
the year ended with $133 in the bank and nowhere for it to go.

So: three things to want, each doing a different job.

| item | price | what it does |
|---|---|---|
| a second crash pad | $260 | removes the penalty for climbing high above bare ground |
| a hangboard | $85 once | trains fingers on a day you cannot climb, for skin |
| a gym membership | $75 / month | the only climbing that ignores the weather |

Twelve seasons per policy, 365 days, rest-until-skin 3.0. `careful` owns
nothing beyond the one pad everybody starts with; `kitted` buys all three.

## The result, and it is not the one I expected

| policy | sends | crag days | burns |
|---|---|---|---|
| careful (buys nothing) | 38 | 1,568 | 6,227 |
| kitted | 43 | 1,207 | 5,397 |
| kitted, both pads from day one | **59** | 1,233 | 5,514 |

The kit as bought is **neutral** — 43 against 38 is inside the per-seed
variance, which ranges 0 to 18 sends. But forcing the second pad is +37% over
kitted and +55% over careful, at *the same number of crag days and burns*.

That difference is the whole finding, and it generalises:

> **Skin caps the year, so kit that spends skin can only move climbing
> around. Only kit that raises the value of the skin you already spend can
> add to a season.**

The pad costs no skin. It makes every burn you were going to take anyway more
likely to land. The hangboard and the gym both spend from the same fixed
budget the crag draws on, so at best they relocate a season rather than
growing it — which is exactly what the crag-days column shows: kitted climbs
361 fewer days outside and comes out level.

## The hangboard is not a trap, but using it badly is

The first run had the board fire on any dead day. Result: **0 sends across
five seasons**, against a non-owner's 12. It ran 122–177 days a year, spending
1.6 skin a time on the skin the crag was waiting for.

Gate it on genuinely surplus skin (>7.5, i.e. skin the weather was going to
waste anyway) and the same item, at the same price, gives **24 against 12**.
Same object, opposite sign, decided entirely by when you reach for it. That is
a good mechanic: there is a right and a wrong way to use it, and the wrong way
is the tempting one.

## Bugs and regressions this measurement caught

- **Nothing ever expired the membership.** `KitDay` existed and was called by
  nobody, so the probe reported 365 days of membership bought with a single
  $75. It ticks down in `SleepToNextDay` now, with everything else that runs
  out.
- **The board out-trained the wall 4×.** At `hangboardFingerGain` 0.22 with no
  cap on sessions, skin alone allowed eleven hangs a day: 1.44 finger points
  against climbing's 0.32. Now one session a day at 0.06 — about 40% of a
  light session, which is what "worse than climbing" has to mean numerically.
- **Pads made the game harder for everyone.** `Kit.pads` defaulted to 0, so
  adding pads silently applied a penalty nobody could afford to remove: the
  five-seed send count fell from 17 to 3. That is not a new decision for the
  player, it is the game quietly getting worse. Everybody starts with one pad;
  the second is the purchase.
- **The probe bought a membership it could not use.** Renewing whenever it
  lapsed bought 126 days and used 27. A winter membership — the only kind
  anyone actually buys — is what a person does, and reporting "the gym is a
  bad deal" while the probe was shopping badly is how you tune away a
  mechanic that was working.

## What it says to do next

The second pad is the one item measured to move the outcome, and the probe
**never once afforded it** across twelve seasons: $260 plus a $150 float
against a year whose cash high is around $240. That is not a bug. It is the
first time in this project that the thing worth having is the thing you
cannot quite reach — which is the pressure Phase 3 has been looking for since
it started.

The gate is still not passed: `kitted` is level with `careful`, not ahead. But
the shape of the answer is now known rather than guessed. What closes it is
more kit of the pad's kind — leverage on skin already spent, not more ways to
spend it. Candidates in that shape: better rubber as a purchase rather than a
repair, beta bought from a partner, and a rest-day session that trains
something skin does not pay for.

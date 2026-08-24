# Phase 11, pass 1 — a life outside it

*2026-08-24*

The 2D game is a **life**-sim. The port, until this commit, was a climbing
sim with bills: every hour in it was an hour of climbing, an hour of paying
for climbing, or an hour of recovering from climbing. Phase 11 is the bucket
of everything else, and the roadmap lists fourteen things in it — romance,
family, music, reading, the garden, fishing, cooking, crafting, grime,
propane and water, bivy spots, hitchhikers, the caravan, and locals who
remember you.

Fourteen pastimes is a menu. What makes it a system is one rule, and pass 1
builds the rule with five threads on it:

> **A thread stays in your life because you keep giving it days.
> Stop, and it goes cold. One of them leaves.**

## The five, and what each is for

| Thread | Patience | What it pays | Ends? |
|---|---|---|---|
| **Someone** | 6 days | psyche baseline | **yes** |
| **Home** | 30 days | steadiness above the last piece | no |
| **The guitar** | 21 days | money, scaling with street reputation | no |
| **Books** | 30 days | an hour of rest is worth more | no |
| **The stove** | 14 days | meals cost less and go further | no |

Each pays in the units of a system that already existed, which is what keeps
this file additive: nothing had to be rewritten to let a life in. And
**nothing here touches send odds**, for Phase 7's reason — a life outside
climbing that made the moves easier would be the game telling you how to
live.

## The one thing that is the opposite of rapport

`PartnerBond::rapport` keeps a floor. It drifts down to a fraction of
`everWas` rather than to nothing, because somebody you climbed with for a
season is somebody you climbed with for a season, and acquaintance is
remembered. That fix was two commits ago and it was the third time this
project has found that **a value that ages needs a memory, not just a
slope**.

A thread has **no floor**, deliberately, and it is worth writing down why
the same shape got the opposite answer twice in a week:

- Rapport is *what somebody knows about you*. It cannot go to zero, because
  they still know it.
- Warmth is *whether you are in each other's lives*. It has to be able to go
  to zero, or the thing cannot be lost, and the whole phase is about a thing
  that can be lost.

What is remembered here is `depth` — how far in you got — and its only job
is to price the ending. Six months of turning up costs you six weeks of
being no good afterwards; a fortnight of seeing somebody costs nothing,
because a game that grieves everything grieves nothing.

## The neglect, in numbers

Someone waits six days, then cools at 0.055 a day. From full warmth that is
**about twenty-five days of you being at the crag and nowhere else**. The
status line starts speaking on day seven (*"You have not seen them in 8
days. They have noticed."*) and changes its tune under 0.25 warmth (*"This
is the part where it ends."*).

That is the design: **the ending is never a surprise, it is a slide you
could have stopped.** A thing that ends without warning is not neglect, it
is a die roll.

Family does not leave. The guitar does not resign. All four of the others go
to zero warmth and sit there paying you nothing, waiting for an afternoon —
which is a truthful model of a guitar, and is also what stops the phase from
being five separate ways to lose.

## Where it plugs in

- `SleepToNextDay` runs `LifeNight`, with everything else that counts down
  at night. Three per-day rules in this project have been written and left
  uncalled and every one was found late.
- The psyche baseline `SleepToNextDay` drifts you toward is now
  `PsycheFloor(...) + LifePsyche(...)`. Somebody who does not care what you
  climbed today moves where psyche lives; so, downward, does the month after
  they stop being somebody.
- `BodyContext` carries the life, so **the steadiness reaches a comp too**.
  A phone call home follows you to the Games exactly as a bad tooth does.
- `Rest` takes a `const Life&` **and refuses to default it**, which is the
  one signature in this pass that costs a caller some churn. It is worth it:
  a defaulted life is how you get a second rest path quietly resting at the
  plain rate forever.
- `EatMeal` prices the stove. `SpendTheEvening` is the verb, and it is the
  only work in the game that is also a hobby.

## What the reintroduction pass found

Fifteen defects put back one at a time. Eleven were caught first time. The
four that were not:

- Three were **my injections failing to compile**, not gaps —
  `-Werror=unused-parameter` again, for the third session running. An
  injection that does not build is not a passing test and the script now
  greps for `error:` alongside `FAIL`.
- One was real, and it is the good one: **`TestAFortnightIsNotALoss` could
  not fail.** It ran ninety nights of neglect and then asked whether
  `grieving` was zero — but a grief of five days counts all the way back
  down to zero inside ninety nights, so removing the `enoughToLose` floor
  entirely still passed. The fix is to stop on the night it ends. A test
  that cannot fail is worse than no test, and the reintroduction pass is the
  only thing that has ever caught one.

## What the measurement found

Five seeds, thirty years each, two policies. `lifer` makes the time; the
default keeps up whatever fits in the leftovers and gives somebody a whole
day only when the rock is not in condition — which is the dirtbag's real
priority order.

| | met | lost | grieved | evenings | busking |
|---|---|---|---|---|---|
| default, 5 careers | days 20–37 | **2 of 5**, days 4,146 and 8,901 | 45 days each | 2,366–3,186 | ~$20,100 |
| lifer, 5 careers | days 17–33 | none | — | ~3,427 | ~$20,600 |

**Both careers that lost somebody were among the ones that climbed most**
(617 and 314 days out; the three that kept everybody climbed 151–259). That
is the phase's second "done when" — *at least one non-climbing thread can end
badly through neglect alone* — measured rather than asserted, and it comes
out of the weather rather than out of a rule about the weather.

Four things the probe found on the way there, all of them the tuning being
wrong rather than the design:

1. **Nothing could ever be lost.** Everything was priced at a two-hour
   evening and gated on 21:00, so every thread fitted every night: 1,138
   evenings across thirty years and not one loss. The hours after the crag
   are dead time in this game — the window has gone, the shift is done — so
   a thread that costs an evening costs nothing. Fixed with
   `asksForHours`: seeing somebody takes **six hours**, and it is the only
   entry in the table that competes with a session. That is why it is the
   only one that can leave you.
2. **My own metric was lying, by a factor of eight.** `busked` was a cash
   delta across `SpendTheEvening`, and it came out at $171,851 for a career
   that made $20,646 busking. Read off `BuskingPay` directly it is 489
   sessions and about $690 a year — pocket money against $4,420 of bills,
   which is right. The retune was still needed: at the old $9+$26 those same
   489 sessions would have paid ~$51,000, making a hobby the best-paid work
   in the game.
3. **A first date was free.** With taking-up costing nothing, a career that
   never gave anybody a minute still *had* somebody about 60% of the time:
   meet, coast on full warmth for twenty-six days, lose them at zero depth,
   sit out the cooldown, repeat — roughly a hundred and ninety times in
   thirty years. `SpendTheEvening` now takes it up on the first go and
   charges the same hours, and the engine's second entry point was deleted
   rather than fixed, because a second way in that skips the charge is how
   this happened.
4. **The nag line would not shut up.** `LifeLabel` picked the coolest
   offender, so once a thread hit zero warmth it named that one every night
   for the rest of the career. A guitar that has been in its case for two
   years is not news, it is an inventory. It now warns only while there is
   something left to lose.

**One open tuning question, recorded not fixed:** the life label still has
something to say on roughly a third of nights, even for a career keeping
everything up — five threads and one go a day means something is always a
day late. That may be one warning too eager. It is a feel question and it
wants a screen.

## What is not in pass 1

Fishing, the garden, crafting from scavenged materials, grime, propane and
water, bivy spots, hitchhikers, the caravan — all content on the shape this
pass builds, and all cheap now that `Thread` is a table.

The one that is **not** content and is the phase's third "done when" is
**locals who remember you between visits** — the shopkeeper, the cafe
regular, the person at the desk who greets you by what you did last time.
That is a different data structure (a person with a memory of one fact) and
it is pass 2.

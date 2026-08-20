# Sponsorship, and the fifth time the same wall

Every other source of money in this game competes with climbing — a shift is
four hours you were not on the rock. Sponsorship is the opposite shape:
money that arrives *because* you climbed well. That made it the last untried
answer to the Phase 3 gate.

## The mechanic

> **A shift lands on any day. A photo shoot lands on a good one.**

You cannot shoot climbing photos in the rain, and nobody runs a comp in
February for the love of it. `ObligationToday` returns false on any day
without a window, always — so a sponsor's days are precisely the days you
wanted for yourself. That is the thing sponsored climbers actually complain
about, and the only version of this that is a decision rather than a bonus.

What earns a deal is what people can **see**: what you have sent, what you
have put up, and what the Scene thinks of you. Not ability. A crusher nobody
has heard of gets nothing, which makes the Scene axis load-bearing for the
first time since it was built.

| tier | pays | wants |
|---|---|---|
| shoes | $0 + free rubber | nothing |
| gear | $120/mo + kit | 1 good day a month |
| title | $640/mo | 3 good days a month |

Being hurt is not failing — a long injury pauses the review clock rather
than running it, because a sponsor who dropped you for it would be worse
than most real ones. And a failed review costs one rung, not the career.

## Measured over five-year careers

| | sends | crag days | work % | cash | sponsor $ | their days |
|---|---|---|---|---|---|---|
| unsponsored | 24–26 | 154–429 | 20–21% | $133–186 | — | — |
| sponsored | 24–26 | 237–408 | **16–18%** | $112–195 | $2,520–5,280 | 14–48 |
| **forced title deal** | **24–26** | 239–383 | **0%** | $1,466–3,239 | $39,040 | 130–152 |

Sponsorship genuinely replaces work — a title deal pays you out of the
workforce **entirely**, 21% of days worked down to zero, $39,040 over five
years. And it costs about a month of good days a year.

**The send count does not move.** Not at any tier, on any seed.

## The finding

This is the fifth independent measurement to hit the same wall, and it is
the cleanest statement of it yet:

> A deal that removes work completely and takes twenty-seven good days a
> year leaves the send count **exactly unchanged** — because the binding
> constraint was never money and was never time. It is skin.

A season has 208 days with a window and a player can only climb about 130 of
them before their skin is gone. Losing 27 of the other 78 costs nothing.
Gaining $39,000 buys nothing, because there is nothing to buy that adds
climbing (`notes/phase3-kit.md` — only a crash pad moved sends, and only
because it spends no skin).

**Phase 3's gate cannot be closed by any money mechanic.** Five have now
been tried: bills, fuel, the kit, physio, sponsorship. The next honest move
is one of two things, and it is Evan's call:

1. **Stop skin being the sole cap** — so that time and money have something
   to compete for. Load (Phase 4) is a second budget but it caps the same
   thing from the same side.
2. **Restate the gate.** "Sends per year" may simply be the wrong success
   metric. A sponsored climber who never works again climbs the same amount
   and lives a completely different life — the year's *texture* changes
   totally while its outcome does not. That might be exactly right, and if
   so the gate should measure texture rather than sends.

I lean to (2), and it is not my call to make.

## Tooling, earned the hard way

`patch.py`'s `edit()` raises on a bad anchor, which kills the whole script —
so every edit written after the failing one silently never runs. That has
now produced three broken states in this project: a printf that gained
values but not format specifiers, a migration registered nowhere, and a save
that wrote four fields it never read back. Each compiled cleanly.

`edits()` now checks every anchor first and writes nothing unless all of
them match, so a partial application is not a state the repo can reach.

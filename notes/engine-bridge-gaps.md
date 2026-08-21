# The engine bridge is thinner than the sim

**2026-08-21.** Nine times a function had been written, tested, and called by
nothing. After the ninth (`CoversShoes`) it stopped being worth finding one
at a time, so `tools/check-unwired.py` now asks the question for every public
sim declaration at once: **is this reachable from the engine?**

It found **thirteen more**, and they are not small.

## Why "reachable from the engine" and not "called by anything"

`FactionDay` — the standing drift and the crag-closure countdown — was called
by the season probe *every simulated day* and by the engine never. So the
mechanic measured as working, in detail, for months, while in the actual game
a crag pulled access once and never gave it back.

A harness calling something is not the same as the game calling it. The
checker seeds its roots from `Source/` only, and propagates transitively
through the sim — the probe is deliberately not a root.

## The debt, as it stands

Each of these carries an `unwired-ok: NOT WIRED — …` line at its
declaration, so `check-unwired.py` prints them as debt on every preflight and
the count cannot quietly grow.

**Money the game does not take or give**

| what | consequence in the actual game |
|---|---|
| `FuelFor` | **driving is free.** The changelog called fuel "the only cost that goes up the more you climb" — true in the probe, never true in the game |
| `WorkSalariedDay`, `SalaryDayPay`, `SalaryOwnsHour` | **the salaried-job trap does not exist.** The engine can ask `SalariedToday()` and has no way to work the day |
| `MonthlyStipend` | **a sponsored player is never paid.** The $640/month title tier pays $0 |
| `ReviewSeason` | **a deal is never reviewed**, so no rung is ever won or lost |

**Things other people are supposed to be worth**

| what | consequence |
|---|---|
| `WillBelay`, `BurnsTheyWillHold`, `BestBelayer`, `BelayText` | **"no partner, no pitch" is unenforceable.** `NeedsABelayer` is wired, so the game can ask the question and cannot answer it |
| `PsycheFrom` | a partner lifts no psyche |
| `BetaMultiplierFor` | standing buys no beta |

**And one that is worse than unwired**

`ShoePenalty` is not called by anything — because the resolver prices dead
rubber with **its own inline copy of the same formula**. Two copies of one
question, which is precisely the drift this project has written three
separate rules against. Whichever one is right, there should be one.

## What this does and does not change

**It does not touch any measurement.** Every probe result in these notes —
the Phase 3 gate, the skin sweep, the kit, the body, sponsorship — runs
against the sim directly, where all of this code does execute. Those numbers
stand.

**It changes what "built" means.** Phase 3 and Phase 4 are recorded as having
their named scope complete, and the sim's half genuinely is. The engine's
half is not, and nothing said so, because a `BlueprintCallable` that exists
is indistinguishable from one that is called.

## The rule this leaves behind

The old rule was *anything that happens overnight happens in `Sleep`*, which
came from four ticks that were never called. It was the right rule and it was
too narrow: it only covered per-day ticks, and eight of the thirteen above
are not ticks.

The rule now is the checker. A public sim function is either reachable from
`Source/`, or it says in one line why not.

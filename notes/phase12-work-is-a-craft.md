# Phase 12 — Work Is A Craft

`Sim/DirtbagCraft.{h,cpp}`, SAVE v32, three new gigs, and the shift's own
decision on the jobs board.

## The one sentence

**A lever you pull for money is a lever. A trade you are getting better at
is a life.**

Phase 3 ported what §4 said to port — an odd-jobs board and a salaried trap
— and both were built, wired and measured. What it did not port is that in
the 2D game every job has a craft behind it, and that changes what work
*is*.

## Ten trades, and every one has a gig

Setting, coaching, the counter, labour, trail, the camera, courier, rescue,
the bar, the office. **A craft you cannot practise is a stat**, so three
gigs were added to the board rather than three enum entries — a courier run,
a callout with the rescue team, a shift behind the bar. A test walks 200
days of the board and fails if any trade is unreachable.

Flyering deliberately maps to `Craft::None`. Some work is just work, and
saying so is better than pretending.

## The three gates

### 1. The job you keep changes who your climber is

Every trade pays back into the body or the head — setting is a day spent
reading movement, coaching is a day talking somebody through being
frightened, trail work is six hours carrying rock uphill. Measured, the same
career with the teaching switched off and on:

| | power | endurance | technique |
|---|---|---|---|
| crag-1, work teaches nothing | 22.0 | 48.0 | 65.4 |
| crag-1, trades teach their own | **34.3** | **64.8** | **74.1** |
| north-2, nothing | 11.7 | 49.8 | 62.5 |
| north-2, trades | **35.7** | **64.8** | **75.6** |

Power +12 to +24, endurance +15 to +17, technique +9 to +13 — in the lanes
those trades teach and nowhere else. Fingers goes *down*, because no trade
teaches fingers and a stronger climber picks different routes.

**None of it is as good as climbing**, and that is load-bearing: work that
trained you as well as climbing did would make the salaried trap not a trap.

### 2. A shift has a decision in it

Something comes up on about a third of shifts — a hold spins mid-session, a
kid freezes at the top, the last drop is a hospital and you are forty
minutes down. **Two ways to handle it, and the right one is harder and needs
the craft you have actually built.**

The easy way is never wrong, never punished, and never gets you anywhere. It
has to be a real option or the decision is a skill check. Measured over 60
seeds: somebody who can do the job pulls it off about 50 times in 60;
somebody who cannot manages under 25 — **and never zero**, because a wall is
not a decision.

Deterministic on the day and the trade, so a reload does not reroll it — the
same no-reroll rule every gamble in this game lives under.

### 3. Getting fired outlives the job

Standing is per trade. Botch enough and that employer stops calling, **the
gig comes off your board, and it stays off.**

This one nearly shipped unreachable. The probe's policy only reached when
the craft was there, so it botched 0–2 times in 440 moments and was **never
sacked once in thirty years** — the sacking was testable in the harness and
unreachable in a played career, which is the same as not existing.

Given a policy that always reaches:

| seed | moments | botched | sacked | ends up as |
|---|---|---|---|---|
| crag-1 | 550 | 35 | 1 | setting |
| north-2 | 521 | 31 | 1 | **labour** |
| west-9 | 586 | 32 | 3 | **labour** |

The trade *changes* — losing setting pushes a career into labour, and it is
still there thirty years later. That is the gate: not a number in a menu, a
different life.

## The bug I wrote and the probe found

The work gain went straight onto the skill: `skills.power += gain`. That
made work **the one training path in the game with no headroom on it.**
Thirty years of shifts is about seven thousand hours; at a flat rate that is
a hundred and thirty points into a stat that stops at a hundred.

Through the same diminishing returns as everything else now, and clamped.
Work that out-trains climbing would make the salaried trap not a trap, and a
skill that runs past its ceiling is a bug in any case.

## A function deleted for being sugar

`BoardOpenToYou` filtered a board by who will hire you. It read well, and
nothing used it but its own test — while the engine wrote the same two-line
loop inline.

**A second place for a rule to live is how the rule starts disagreeing with
itself**, which is exactly the class the checker written an hour earlier
exists to catch. The rule is `WillTheyHireYou`, it is in one place, and
`WorkOddJob` enforces it too — so a caller that forgets to filter shows the
gig and cannot take it, rather than walking past a sacking.

## Deliberately simpler than the 2D game

The 2D game tracks employer standing **per building**. This tracks it **per
trade**, because a trade here has one employer behind it. Where two gigs
share a trade — furniture, firewood and night shelves are all Labour — they
share the reputation too. Written down rather than hidden.

Not built: courier accounts, and gym sets that stay up and get climbed by
other people. Both want a place to be, which is Phase 6.

# Getting hurt is something you buy

**2026-08-21.** Evan away from the PC. Phase 4's gate reads:

> *a career plays end to end — a climber arrives, peaks, **gets hurt**, ages,
> and hands the valley to the next one*

So I ran it. Ninety years, four generations, every policy the probe has.

## The probe could not see aggravations, and never could

First finding was mine, not the game's. Every run reported **0
aggravations**, including one with 274 days climbed on while injured.

`ClimbOnIt` fires **per burn**, during the session, and is called from
`DirtbagDay.cpp:251`. The probe snapshotted `injury.severity` and
`daysLeft` immediately before `SleepToNextDay` — which is *after* the day's
climbing — and compared them after. The snapshot was taken after the event
it was trying to detect, so the counter could only ever read zero.

Snapshot moved to dawn, and the comparison moved to before sleep, because
sleep is what heals and a night's recovery would mask an aggravation that
cost more than the night gave back:

| | before | after |
|---|---|---|
| kitted, crag-1 | 0 | **115** |
| kitted, crag-3 | 0 | **60** |

**The mechanic was working the whole time.** Nothing in the game changed and
nothing in the notes had to be corrected — the zero only ever existed in
probe output. But it is the fourth measurement bug this project has found by
re-reading its own instrument rather than the result, after the pad, the
projector, and field 34.

## Injuries are gated behind kit, entirely

Ninety years each, crag-1:

| policy | buys a board + membership | injuries | days hurt |
|---|---|---|---|
| greedy | no | **0** | 0 |
| projector | no | **0** | 0 |
| stakeout | no | **0** | 0 |
| salary | no | **0** | 0 |
| sponsored | no | **0** | 0 |
| **saver** | **yes** | **13** | 112 |
| **kitted** | **yes** | **22** | 587 |

Not a tendency — a switch. **Every policy without kit gets exactly zero
injuries across ninety years and four lifetimes.**

The mechanism is load, and it is coherent. `stakeout` climbs **1,994 outdoor
days for 5,652 burns**; `kitted` climbs **fewer outdoor days — 1,663 — for
11,304 burns**, because 2,743 gym days and 7,989 board sessions happen on
days the weather refused. Load builds with how hard you pull and decays with
rest, and outdoor climbing is so thoroughly rationed by weather, skin and
work that **load never accumulates faster than it sheds**: average load 0–1
without kit, 38–41 with it.

## Which is either the best thing in the phase or a hole in it

**The case that it is right.** It says something true and specific: the
dirtbag who only climbs when the rock is good never gets hurt, because the
weather is their training plan. You have to *buy* your way into enough
volume to injure yourself — a membership and a board are what let you climb
through a winter, and that is exactly what wrecks fingers in life. It also
ties the money loop to the body loop, which is Phase 3's restated gate
(*"money decides how good your burns are"*) paying off in a place nobody
designed it to.

**The case that it is a hole.** Phase 4 names injuries, physio, aggravation
and aging as its scope, and its gate says a career includes getting hurt. A
player who never buys a membership plays a twenty-three-year career and
**never meets any of it** — no injury, no physio bill, no decision about
whether to pull on. A named phase mechanic sits behind an optional purchase,
and nothing tells the player that is the trade they are making.

**I have not changed a dial.** Which reading is right decides whether
anything needs doing, and that is a design call, not a measurement.

## What I would do

Neither extreme. If outdoor-only climbing should be able to hurt you, the
honest lever is not the injury chance — it is that **load sheds too fast for
a rationed schedule to ever stack**, and that is one dial with a clear
meaning. A player who climbs three days running in good weather should carry
something into the fourth.

But the gate can also just be read as passing: the career *can* include
getting hurt, and whether it does is a consequence of how you spend money.
That is a defensible game. It only needs the player to be able to see the
trade.

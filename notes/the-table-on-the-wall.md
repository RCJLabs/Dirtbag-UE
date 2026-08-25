# The table on the wall

*2026-08-25*

`Sim/DirtbagGymLeague.{h,cpp}` new, SAVE v46. `GYM-10` ported from the 2D
source, and with it the gym family is complete: `GYM-1` through `GYM-12`,
minus the two the source itself never wrote.

## Not the league this port already had

`Sim/DirtbagLeague.h` has been here for months: the low end of the comp
system, five dollars on a Wednesday at somebody else's gym, five problems,
and a personal best you chase forever. You climb it.

This one you do not climb at all. The source's own comment draws the line:
the existing leagues have covered Send City and The Cave since long before
you owned anything, and comp night is a one-off on a cooldown. **Neither is
an institution.** A league is the same night every week, the same people, a
table that carries, and at the end of a run somebody's name on it.

Two systems, one word, and the headers of both now say so.

## The format is the decision, and it decides who wins

A board ladder belongs to whoever trains. A handicap league belongs to
whoever improved. A team league belongs to whoever turns up with people. One
go on one route belongs to whoever holds their nerve.

**Every one of the thirteen regulars leans one way**, so the format is a
statement about who your gym is for, and the standings prove it week after
week. This is the first thing in the port that reads the cast `GYM-2` and
`GYM-5` built for something other than a line of prose.

And the two decisions can disagree. A hardcore room's cohort — Kestrel, Dom,
Rafferty — contains **no `Social` at all**, so a team league there has
nobody in it who suits the format. That is not a bug. It is a set mix and a
league night contradicting each other in public, on a table on the wall, for
six weeks. `TestTheRoomYouBuiltAndTheNightYouPickedHaveToAgree` asserts it.

## You are not in the standings

`gymLeagueEntrants` is commented *"whichever regulars have a story with you,
plus you"* and returns the regulars. The code is right and the comment is
wrong — **the third comment/code mismatch found in this family this week** —
and the code is right for the same reason `GYM-9`'s host does not get a
scorecard. You run this. Your name on the table would blur the one thing the
table is for.

Entrants need one arc stage lived. Somebody you have never spoken to is a
membership, not an entrant, which is what makes the league something the
floor earns rather than something the building comes with.

## The bug the measurement found

The source keys a night's score on the league's name, the week and the
climber. The week resets to one when a run ends.

So **every run of a league scores identically**: the same six nights, the
same table, the same winner, forever. Measured over 251 runs of one league
across thirty years:

| | before | after |
|---|---|---|
| different names on the wall | **2** | **7** (everybody who turns up) |
| runs won by somebody the format suits | 1 of 251 | 167 of 251 (**66.5%**) |

That is precisely the failure the source records tuning `GYM_LEAGUE_SUIT_BONUS`
away from — *"the climbers a format suited won 100% of runs and only three
people in the building could ever hold the trophy"* — arriving through a door
the tuning could not see. A champion distribution measured across *different*
leagues looks healthy while every run of *one* league is a rerun.

The fix is the run number in the seed. 66.5% is where the source says it
wanted the tilt (~62%): favourites and upsets, which is a league.

## What it measures out at

Thirty years, six seeds, a hoarder who owns the room and runs a handicap
league every week:

| | |
|---|---|
| league nights run | 1,512 |
| six-week runs completed | 251 |
| taken at the door | $218,603 |
| different champions | 7 of 7 |

About **$145 a night**, three hours and ten energy, on a fixed night you
chose. It is not the money — the gym's own books move more than that in a
fortnight. It is that there is a table on the wall and somebody has been
turning up to it for twenty years.

## The family, finished

`GYM-1` the P&L engine, `GYM-2` and `GYM-5` the people, `GYM-3` the staff,
`GYM-4` the town, `GYM-6` the year and what goes wrong in it, `GYM-8` the
youth team, `GYM-9` hosting the circuit, `GYM-10` the league, `GYM-12` the
wings. `GYM-7` and `GYM-11` do not exist in the source.

Every wing column now has a reader: `target` and `upkeep` from the first
pass, `cafe` from `WhatTheWingsEarn`, `youth` from `GYM-8`, `bid` from
`GYM-9`. Nothing in that table is a dial that lies any more.

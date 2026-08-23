# Phase 9, pass 1 — the comp

**2026-08-23.** Evan: *"Time for phase 9"*.

Phase 9 is seven things: the gym comp, the circuit season, three tiers, a
field of people, the national ranking, the national team, and the World Cup
and the Games. **This is the first of them, plus the two it cannot exist
without** — the field and the tiers — and the ranking number the rest will
read.

## The format is the point

A gym comp is **five problems, seven attempts across the whole session**. At
most two can be worked and the rest are one-shot, so the first decision of a
comp is *which two you believe in*. That is a resource-allocation minigame
played with verbs the session resolver already has, which is CLAUDE.md's
core design call word for word — **2D minigames, 3D staging**. A comp that
resolved to a number would be a table lookup with a rosette on it.

The board is a spread, not five copies of one grade: two below the tier's
level, two at it, one above, priced accordingly. The hard one pays most and
might take three of your seven.

Nerves are one dial. `pressure = 0.80` keeps 80% of your normal margin,
applied as an odds penalty rather than a worse climber — **the climber is
the same person; it is the situation that is harder.**

## The field is a redistribution, not a difficulty setting

Seven named climbers at stable offsets, each with **a discipline they are
known for and one they are soft on** — a grade harder and a grade softer.
Every one gets exactly one of each and a board spreads across the types, so
the field's average strength is unchanged. What changes is that results start
to mean something: **Kai takes the dyno problem off you every time and you
take the crimpy one back off him.**

## Two real defects, both found by measuring rather than testing

**The field had to be shifted by tier.** Without it, flashing an *entire*
local board won **16 times in 60.** Kai sits a grade above you; a competitor
a grade above the board flashes everything on it, so **their ceiling equals a
perfect round's and form decides the winner.** You could not beat them by
climbing better — only by them having a bad day. That is not "Local is yours
to lose", it is a coin toss with extra steps.

Shifting the whole field by tier is what the 2D game does (its `COMP_TIERS`
carries a field column) and it is the honest fix: the same seven people, a
weaker crowd at the local one.

| localFieldShift | wins of 60, flashing everything |
|---|---|
| −0.5 | 48 |
| −0.75 | 48 |
| **−1.0** | **48** |
| −1.25 | 48 |
| −1.5 | 60 |

**The dial is coarser than it looks.** Four values give the identical answer,
because grades are integers and a competitor's effective level is compared
against them in half-grade bands — the shift only does anything when it
crosses a boundary. −1.0 is the middle of that plateau and reads as what it
is: the local crowd is a grade weaker than the one you meet at Regional.

At −1.0: **48 wins and 60 podiums of 60.** A perfect local round takes it
four times in five and Kai still steals one, which is the difference between
a comp and a formality.

**And a zero score no longer takes ties.** *"You take ties"*, plus a stable
sort, plus being pushed onto the board first, meant a climber who got
**nothing** up at a comp two tiers above them sorted ahead of everybody else
who also got nothing, came **fourth of eight** and collected top-half prize
money for it. A tie at zero is not a tie; it is a room full of people who did
not climb. Fixed twice: the first attempt returned `false` for zeros, which
left the stable sort's insertion order intact and changed nothing.

## What a comp pays

Deliberately little. **Comp prize money is not how a climber eats** — the 2D
game's own note records the circuit becoming economically pointless when it
tried to be. $120 for a win against a sponsor stipend that pays more per day.
What a comp pays in is **national ranking points**, which are what the tiers
gate on and the only thing that lasts.

Beating the rival is worth its own bump on top of wherever you placed, in the
same currency a first ascent moves — because it is the same question.

## The doors

**A poster on the gym wall, three days out.** The warning is the mechanic: a
comp you find out about on the day is a dice roll, and one you can see coming
is a week of deciding whether to rest for it. Three days is enough to skip a
session and not enough to train for it. Firm dates on a fortnightly cadence
rather than a roll, because **a schedule you can plan around is the whole
difference between a comp and a random event.**

**E signs in** — six hours, the entry fee, thirty energy. **1–5 spend your
goes**, and the wall's panel becomes the board while you are in it: what each
problem is worth *to you now* (a flash is off the table the moment you have
touched it), how many goes it has taken, and where you got to. The fifth
problem needed a fifth key, because a board with a problem you cannot press
is the bug the ethics prompt had two days ago.

It turns itself in when the last go is spent. There is nothing left to
decide, and making the player press one more key to hear a result they cannot
change is ceremony.

**The live comp is not saved**, and that is deliberate: you are in the gym
for six hours and the save is written when you sleep, so a comp interrupted
by an alt-F4 is a comp you did not finish. Storing it would mean deciding
what a half-comp means on load, which is a worse answer than *"you missed
it"*.

## What is not here

The six things that sit **on top** of a comp, all of which read a
`CompResult` and none of which changes what a comp is:

- **the circuit season** — five comps on firm dates, the last a 1.5× finals,
  with a champion, a runner-up and a bronze, and a no-show handing the rival
  sixty points
- **quals → semi → final** at National and above
- **the national ranking's six tiers** — the points exist and are saved; the
  named tiers and what they unlock do not
- **the national team** — five teammates, a head coach with opinions, a $180
  taxable stipend, a review that can take it back
- **the World Cup** — ten venues, travel costs, and a field that flies
  whether you do or not
- **the Games** — a 56-day cycle, declared disciplines, problems a grade
  above yours

Plus **leagues**, the low end of the same system.

The ladder is the phase; this is the rung everything else stands on.

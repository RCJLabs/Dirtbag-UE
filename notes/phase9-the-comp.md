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

---

# Pass 2 — the circuit season and the ranking ladder

**2026-08-23, same day.** Evan: *"Take circuit and tiers now"*.

## A season has a shape

Five comps on firm dates six to eight days apart, and **the last one is the
finals** — not a sixth event but the same format worth half as much again.
That multiplier is the only thing that makes peaking for a date a decision.

**Firm is the point.** A schedule you can plan around is the whole
difference between a comp and a random event, and it is what makes *not*
turning up a decision rather than an accident.

## Everybody scores

A table that only tracked your points would be a personal best with other
names printed near it. The field and the rival bank from their own placings
every comp, so the standings are a season rather than your season — and the
rival can win one off you while you are away at a crag.

## The placement curve, and the number it replaced

Pass 1 fed the ranking with the comp's prize `rep` — 10 for a win. Against
tiers that gate at 350 and 1200, that is thirty-five wins to reach Regional.
The real curve:

**1st takes 100 and the back of the field still takes 5**, linear in *how
many people you beat* rather than in where you finished. A big field is worth
more to win, which is what makes stepping up a tier attractive rather than
merely harder.

The prize rep did not go away; it went where it belonged. **`rep` is what the
scene thinks and feeds standing; ranking points are what the federation
records.** Two different numbers about the same afternoon, and conflating
them was the pass-1 shortcut.

## Six tiers, with somebody else's numbers on purpose

Unranked · Regional Climber (120) · National Prospect (350) · **National Team
(700)** · **Olympic Hopeful (1200)** · World-Class (2200).

Those two are pinned by a test rather than left to drift, because they are
load-bearing for systems that do not exist yet: 700 is where a national team
calls you and 1200 is where the Games become reachable. Moving them later
moves two things silently.

## What not turning up costs

The rival banks **sixty** and you lose standing. The asymmetry is the
commitment — a firm schedule you can ignore for free is a suggestion.

## Two real bugs found while wiring it

**The forfeit fired the day after every comp, including the ones you won.**
The night tick checked only `compsDone < compsPerSeason`, and after your
first victory one done is still fewer than five. **What separates a comp you
climbed from one you skipped is not the date — both are in the past — it is
whether the ledger caught up with the calendar.** `CompsDueBy` is that count,
and the fix forfeits the *difference* rather than firing on a date.

**And `CloseSeason` had a guard that was never the mechanism.** Each podium
branch also checked "you scored something", which reads as prudence and was
dead: `SeasonTable` already sorts a zero behind every other zero, so a career
that entered nothing is last and never reaches those branches.

Removing the guard **changed nothing and failed no test** — which is the
honest signal that it was never doing the work. Two mechanisms for one
invariant is how they drift, so the redundant one went and the tie-break
kept its test.

## Doors

The poster and the sign-in are pass 1's. New: **the season standings and
your rank** in the HUD's slow lines — a rank is a state and not an event —
and both are **silent until you have entered something**, because *"Unranked,
120 to the next"* on day one is a progress bar for a system the player has
not met.

A season closes the moment the finals are turned in rather than at Sleep,
because the table is complete then and hearing about it tomorrow morning
would be the game telling you something you watched happen.

SAVE v25 carries the season, dates and all — without it you would wake up in
a season with no schedule and the night tick would forfeit its way through
the year.

## What is left of Phase 9

- **quals → semi → final** at National and above
- **the national team** — five teammates, a head coach, a $180 taxable
  stipend, a review that can take it back
- **the World Cup** — ten venues, travel costs, a field that flies whether
  you do or not
- **the Games** — a 56-day cycle, declared disciplines, problems a grade
  above yours
- **leagues**, the low end of the same system

All four of the remaining systems read the ranking number this pass just made
real. That was the point of doing it second.

---

# Pass 3 — the national team

**2026-08-23, same day.** Evan: *"Do it"*.

`NATL_TIERS` has read **"National Team" at 700 points** since the ranking
existed, and until now it was a word on a progress bar. The 2D game's own
note is the brief:

> *"A national team is not a threshold. It's a roster with your name typed
> on it, five other people who are also on it, a head coach who has opinions
> about you, a stipend that doesn't cover rent, and a review at the end of
> every domestic season that can quietly take all of it back."*

## Two lines, not one

Clear **700** to be named. Once you are on, you hold all the way down to
**560** — the grace a selection committee actually gives a returning
athlete, and without it a season spent hovering around the number is a coin
flip you take five times.

The test for this started as `holdAt < selectAt`, which is a tautology: it
catches the dial being collapsed and nothing else. The real check is that
**the same number treats an insider and an outsider differently** — at
exactly 560, somebody already on the paper stays and somebody outside is not
let in. Verified by collapsing the dials and watching *that* fail rather than
only the comparison.

## Your teammates are people you already know

The roster is **the top of the national field** — the exact climbers you have
spent a career chasing up the standings, not five names invented for the
occasion. That is what makes being named feel like arriving somewhere rather
than being handed a menu of strangers.

A teammate is a person before they are a number, so each carries a role read
off what they are known for and which way they are going: **the crimper, the
engine, the junior, the veteran.** The roster is rebuilt at every review, so
people come and go under you whether or not your own status changed — and the
game keeps the names of the ones who are not on it any more.

**The climber you went past to get on it is recorded.** They know.

## Getting the call once never un-happens

Being cut takes the spot, not the fact. `everNamed` survives it, the HUD line
changes from *"On the national team"* to *"Off the national team after three
seasons"*, and coming back is worth less than arriving was — because it is.

The review says so without editorial: *"The committee did not name you this
year."* A committee does not explain itself and neither does this.

## The units bug, caught before it shipped

The 2D game's rep values here are **16, 6 and 6**, on its own `rep` scale
that runs to hundreds. The port's standing runs **−1..1**, and
`Sim/DirtbagFactions.h` says outright: *"0.1 is a small deliberate act, 0.3
is a big one."*

Carrying those across unconverted would have shifted the Scene by **sixteen
on a scale where one is the whole range**. That is the rival's
grade-for-skill bug wearing a different hat, and it is available **every time
a number crosses between two systems that both call their axis
"reputation".** The conversion is written into the dial comment rather than
done silently.

**0.40 / 0.15 / 0.15**, with 0.40 deliberately *above* what the factions file
calls a big deliberate act — being named to the national team is the largest
single thing that can happen to a competitor's standing with the scene.

## When the committee sits

At a circuit season's close and nowhere else. **A domestic season is the unit
a selection committee actually works in**, and reviewing you every night
would make the team a thermostat. It runs after the season's own podium
points are banked, because those are part of the year the committee is
looking at.

SAVE v26 carries the roster, the coach and who you went past. Without it a
career would be **re-announced onto the team at every season's close,
forever**, with a different coach each time.

## What is left of Phase 9

- **the World Cup** — ten venues with travel costs, and a field that flies
  whether you do or not
- **the Games** — a 56-day cycle, declared disciplines, problems a grade
  above yours
- **quals → semi → final** at National and above
- **leagues**, the low end of the same system

The ranking number gates both of the first two, and it is real now.

---

# Pass 4 — the World Cup and the Games (2026-08-23)

`Sim/DirtbagWorldStage.{h,cpp}`, SAVE v27, and the gym wall as the door for
all three ladders.

## The one design call

**The world stage is absolute and a domestic comp is not.**

A gym comp is set at *your* grade plus a tier offset, because a gym comp is
your peers — the board follows you up as you improve. The World Cup does
not. It is set where it is set, the field climbs what it climbs, and getting
better is what closes the gap.

The first version got this backwards, and the result is the most complete
failure this project has produced so far: **the entire top of the ladder was
unwinnable at every skill level in the game, for everyone.** A climber at 95
skill topped 0.01 of five problems and came thirteenth of thirteen. So did a
climber at 55. Every test passed. Nothing was broken; the system simply
could not be played, and no assertion in a 76,000-check suite noticed,
because they were all orderings and orderings hold fine on a game nobody can
win.

Found by measuring, exactly like the rival's grade-for-skill bug in Phase 8.
The lesson is the same one and it has now cost two passes: **a system whose
tests are all relative needs one test that is absolute.**

## The numbers, and where they came from

`worldStandard = 8.5`, set against what a career actually reaches rather
than against a feeling. A ten-year career at the crag lands on an allround
grade of about 6.9 (greedy 6.92, kitted 6.98, sponsored 6.84 over 3650
days), so the world stage sits a grade and a half above where a good outdoor
decade finishes.

Measured over 300 rounds per rung:

| your grade | World Cup place (of 13) | tops (of 5) | podium | win |
|---|---|---|---|---|
| 9.2  | 10.0 | 1.9 | 0% | 0% |
| 9.9  | 5.8  | 3.2 | 19% | 4% |
| 10.6 | 2.5  | 4.3 | 77% | 39% |

Steep, because the odds curve is steep — the whole gap is a grade and a half
wide and every tenth of it shows.

The Games were tuned the other way and had to be corrected. At
`olympicGradeBump = 0.5` a grade-9.9 climber medalled at **44%** while
podiuming a World Cup at 19%, because a podium is three of eight there and
three of thirteen here. Level boards put it at 18% and 19%, which is the
intended shape: **a medal is as hard as a World Cup podium, and getting to
the start line is what makes it rarer.**

## Two calendars that were badly wrong, and the probe that found them

Nothing in the harness could see either, because both are about *how often*
and a harness assertion asks *whether*.

- **The Games came round every 56 days.** The probe entered **sixty Games in
  a ten-year career.** Now 1460 — four years, the cycle everybody knows. A
  thirty-year career sees seven; most see two or three, which is what makes
  an appearance worth having on its own.
- **A World Cup season lasted eleven weeks.** Six rounds four days apart plus
  a 45-day break ran **forty-seven seasons and two hundred and eighty-one
  rounds in ten years.** Now fourteen to twenty-three days apart with a
  240-day off-season: about a hundred days of competing and the rest of the
  year training for it.

Both were found by teaching the probe to play the ladder, which nothing had
ever done — see below.

## The probe had never entered a comp

`Sim/tools/season.cpp` gained a `comper` policy. Until this pass **the
entire comp system — the gym comp, the circuit, the ranking tiers, the
national team, the World Cup and the Games — was measured only by the
harness.** The measured game was not the played game at the scale of a whole
subsystem, and the door checker could not see it because every door existed;
they were simply never opened by anything that plays a career.

`comper` signs in at every comp it can pay for, plays the board easiest
first, gets on the plane when the federation is paying and the money is
there, and starts at the Games when it qualifies. The preflight coverage
count went from 103 sim rules to 127.

## What it found that is not this pass's to fix

Reported rather than changed, because these are Phase 9 pass 2's and pass
3's dials and moving them re-balances numbers that were tuned deliberately.
A ten-year `comper` career, seed `crag-1`:

- **Ranking peak 14,633 against a top tier of 2,200.** World-Class is
  cleared inside the first two years and the ladder has no top after that.
  The Olympic gate (1200) and the team gate (700) are cleared in months, so
  neither is really a gate.
- **The committee sat fifty times in ten years** — a domestic season is about
  73 days, so the national team is reviewed five times a year. A selection
  committee that meets every ten weeks is a thermostat, which is the exact
  thing pass 3's note said it was avoiding.
- **Zero wins in 247 domestic comps.** The tier gate promotes on ranking, and
  inflated ranking parks a grade-6 climber permanently at National tier,
  where the board sits two grades above them forever.

All three are the same root: **ranking points accumulate on a domestic clock
that runs about five times a year.** The fix is one of `compsPerSeason`,
`seasonBreakDays`, or `RankingPointsFor`, and which one is a taste call.

## What is left of Phase 9

- **quals → semi → final** at National and above
- **leagues**, the low end of the same system
- the domestic clock above

## The door

All three ladders post on the gym wall and all three open with the same key,
in the order the day matters: **the Games, then a World Cup round, then the
Tuesday comp.** Two can only collide by coincidence of the calendar, and
when they do the Games win — nobody skips them for a Tuesday.

`CanFly` and `CanEnterTheGames` return the whole answer — can you, which
round, what it costs, and why not in the game's voice — from the sim rather
than the UFUNCTION, because **a rule that lives in an engine door cannot be
tested from the harness**, and every rule this project has left in one has
been found late.

# Phase 8 — somebody to beat

**2026-08-23.** Evan: *"Continue phase 8"*.

## §4 ports the route and calls it the person

`concepts/DIRTBAG.md` §4's port list says *"projecting/nemesis tracking"*,
and that has been built for weeks: `DirtbagDay.h`'s `nemesis` is **the unsent
line you have fed the most burns**. It is a good system and it is not this
one.

The 2D game also has a **rival**: a named climber who was already here when
you arrived, whose grade tracks a step ahead of yours, who **takes first
ascents of open lines and keeps their name on them**, who ages on their own
clock, who eventually retires *into* the scene rather than out of it, and who
is replaced — because somebody always steps up.

The port had partners, factions, a crew, a scene and a guidebook, and nobody
to lose to.

## Two things it does not duplicate, on purpose

**Taking a line runs through the Lot's own machinery.**
`TheyPutUpTheLine(line, who)` is keyed on a *name* rather than a `Partner` —
written that way for the neighbours, and exactly what a rival needs.
`AsAClimber` hands back a `Partner` built from the rival's grade, so
`PartnerTakesFirstAscent` and `TheyPutUpTheLine` write their ascent into the
guidebook by the same path a neighbour's takes. Two rolls for *somebody got
there first* would drift, and the drift shows up as a book that knows about
one and not the other — which is a bug this project has already shipped once.

**Ageing is `DirtbagAge`'s shape, not a second model.** They peak and stop
improving because that is what the player does.

## What makes them personal rather than generated

**Their style leans toward whatever you are weakest at.** Weak fingers draws
a crimp assassin; no endurance draws an engine. They are built to beat you
where it hurts, and a test checks it per weakness rather than once — a single
seed proves nothing about a table.

**They cannot take the top of the ladder.** `gradeCap = 17` means the
mythical grades are yours alone; a rival who could reach V18 would eventually
take the one thing a career is for.

**They chase, they do not teleport.** If you go past them they close the gap
a step at a time, which is what makes going past them mean something — you
get a season of being ahead rather than one frame of it.

## What moves the rivalry

In the 2D game the head-to-head is mostly comps, and **comps are Phase 9**.
So here it is first ascents, which is the currency this build already has:
you get to an open line first and it goes up, they get there first and it
goes down. Not a placeholder — an FA is the highest stake this game has and
losing one is permanent.

## Measured

Thirty-year careers, sixteen seeds, the `projector` policy (the one that
actually goes after open lines).

| | without a rival | with one |
|---|---|---|
| your first ascents | 1.38 | 1.38 |
| lines lost to them | — | **0.44** |
| net open lines taken by you | 1.38 | **0.94** |
| rival generations seen | 0 | **2.44** |

**Every career watches a rival retire** (16 of 16) and **seven in sixteen
lose a line to one.**

## The gates, honestly

**Gate 4 — inherit across generations — passes.** 2.44 generations in a
thirty-year career, each in the record with their age, peak and what became
of them, and the lineage is in order with no gaps.

**Gate 2 — losing a line is felt — passes structurally and is thin in
magnitude.** The line is gone, their name is on it, the guidebook says so,
and it survives a save round-trip. But **0.44 lines in thirty years** means
better than half of all careers never lose one at all. The shape is right;
whether the frequency is is a tuning call with data behind it now, and it is
Evan's rather than mine.

**Gate 1 — the rival changes what you choose to climb — does NOT pass, and
the probe cannot pass it.** Your first ascents come out at **1.38 with or
without a rival**: they remove lines from the world, and nothing makes you
*behave* differently, because every probe policy is a fixed heuristic and
none of them reads the rival. Changing what you climb needs a **race** — the
2D game has one (`race`, with an `fa` flag for racing to a first ascent) —
or a human who can see them working something. Recorded as outstanding
rather than argued around.

**Gate 3 — the ally arc, and declining it is a real choice — does NOT pass,
and this one is my fault.** `WouldPartnerUp` is wired, but the engine sets
`allied = true` in the same breath: **you cannot decline.** That is the
no-door bug with the door installed and nailed open. It needs a prompt with
two keys, like every other choice in this build, and it is not here.

## Verified by reintroduction

Eight defects, each put back and counted:

| defect | caught |
|---|---|
| the metronome: they never age, they tick forever | 1 |
| they teleport to your grade instead of chasing | 2 |
| no cap — they can take the mythical grades | 1 |
| a per-day retirement roll instead of per-season | 2 |
| a friend vanishes (the allied branch loses its case) | 1 |
| their style stops reacting to you | 1 |
| the night tick does nothing | 4 |
| the save drops the lines they took | 1 |

## Four findings about the tests rather than the code

**A reintroduction that does not compile is not a passing test.** Killing
`StyleAgainst` tripped `-Werror=unused-function`, so the suite never ran and
printed no failures — which reads exactly like "the test does not catch it".
CLAUDE.md warns about this and it still took two attempts to notice.

**A test that segfaults tells you less than one that fails.** The save test
asserted `firstAscents.size() == 2` and then read `[0]` regardless, so
dropping the count from the save crashed the harness and took the rest of the
suite with it. Guarded now.

**`RivalDials::startAge` shared a name with `AgeDials::startAge`** — 26 and
24 — and the dial checker read a deliberate two-year gap as one number that
had drifted. It was right to: a reader has no way to tell a deliberate
difference from an accidental one when the fields are called the same thing.
Renamed `rivalStartAge`, with the two-year gap written down as the reason
they are a benchmark: **their lead is a head start rather than talent.**

**And the one that matters.** `AsAClimber` passed the rival's **grade**
(0–18) to a parameter expecting a **skill** (0–100). The rival was a
beginner. They took **zero first ascents in thirty years** while ageing,
peaking, retiring and being succeeded entirely convincingly — and **nothing
in the suite noticed**, because every test asked whether the machinery ran
rather than whether it did anything. Found by measuring, not by testing. The
inverse conversion is now `GradeToSkill`, written beside `SkillToGrade`
rather than open-coded, because two copies of one conversion is how this
happens.

**The probe now runs the rival**, for the reason `check-parity.py` exists:
the engine's `AdvanceTheLot` takes lines for them, so a probe that did not
would make every Phase 8 measurement a measurement of a game nobody plays.

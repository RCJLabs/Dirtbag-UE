# The first career probe, and the three things it found

**2026-08-21.** Phase 4's gate is *"a career plays end to end — a climber
arrives, peaks, gets hurt, ages, and hands the valley to the next one — and
the guidebook at the end reads like somebody lived there."*

Nothing had ever simulated more than one lifetime. The season probe runs one
immortal climber; `TimeToThinkAboutIt`, `TallyCareer` and `Inherit` were
reachable only from the engine, and no human has played thirty years. So the
probe grew a `careers` mode: retire when the game offers, inherit, keep
going, and print the guidebook at the end.

It found one bug of mine and two of the game's. The game's were worse.

## 0. My bug, first, because it nearly hid the others

The first run showed a climber going **V5.0 → V-1.0** over thirty years and
made it look like the aging model was catastrophic. It was not. I carried
`player.day` across the inheritance with a comment explaining that age is
derived from the day counter — which is exactly why carrying it is wrong.
`Inherit` sets `day = 1` deliberately. **The inheritor was being born at the
age their predecessor retired.**

Worth keeping because the shape recurs: a confident comment justifying a
line that does the opposite of what the comment says.

## 1. Inherited climbers were born with nothing

`Inherit`'s comment: *"the next climber is twenty-four with a fresh body and
nothing in the fingers… Skills stay at whatever a new PlayerState starts
with."*

What a new `PlayerState` starts with is a **zeroed struct**. All five skills
at `0.0`. And the engine's mirror `FDirtbagClimber` defaults every skill to
`50.0` in its own header — so the two halves of the same idea disagreed, and
`FromSim` copied the sim's zeros over the engine's fifties.

The second generation of every career therefore:

- **could not send a V0.** Measured: 45 attempts on *Roadside Attraction*,
  the warm-up boulder, over two decades.
- **could never be offered retirement**, because that test requires a peak
  grade above zero and they never had one. They climbed to **85 and past
  it**, and the run ended with them still going.

`kStartingSkill` and `NewClimber()` are the fix: one named number, used by
`Inherit`, and the honest meaning of "nothing in the fingers" — *a
beginner*, not *a zero*.

### The test agreed with the bug

`TestTheWorldRemembersAndTheBodyDoesNot` asserted:

```cpp
CHECK(next.climber.skills.power == fresh.climber.skills.power);
```

…where `fresh` was a default-constructed `PlayerState`. Both sides were
zero, so it passed, for years, while describing a defect. **A test that
compares the thing under test against the same mistake is not a test.** It
now checks against `NewClimber()` and adds the two properties the old one
could not see: an inheritor can climb, and an inheritor can eventually stop.

## 2. The valley did not remember

With the body fixed, ninety years gave four generations — and this:

```
Line 752,  V4. FA Climber 1   [key: the short wall behind the cattle grid]
Line 1139, V3. FA Climber 1   [key: the slab right of the pull-off]
Line 357,  V4. FA Climber 2   [key: the short wall behind the cattle grid]
Line 874,  V3. FA Climber 2   [key: the slab right of the pull-off]
Line 421,  V4. FA Climber 3   [key: the short wall behind the cattle grid]
Line 655,  V3. FA Climber 3   [key: the slab right of the pull-off]
Line 789,  V4. FA Climber 4   [key: the short wall behind the cattle grid]
Line 885,  V3. FA Climber 4   [key: the slab right of the pull-off]
```

**Four people each did the first ascent of the same two boulders.**

`Inherit` wipes `player.projects`, which is right — the ledger is personal.
But `WriteIntoTheBook` derived the whole book from that ledger, so a line
your predecessor named was an open project again the moment they retired.
The legacy note's claim — *"your lines are in the guidebook under the names
you gave them"* — was true only inside one lifetime.

`WriteIntoTheBook(CragLine&, const NamedLine&)` fixes it: a `Legacy` already
carries the key, the given name, the confirmed grade and who they were,
which is everything the page needs. The engine writes legacies into the book
before the living player's ledger, every time the crag loads.

After: **2 named lines across 4 lives**, and the later generations find them
already done.

## 5. The valley is not exhausted — measured properly, after two wrong turns

The §3 reading below — *"one person lived there and three more failed"* —
was taken on the build before the Lot named anything and before commitment
protected a line. Re-measured on the current build, and the answer is
different. It took two wrong turns to get to, both worth recording.

**Are the remaining projects reachable at all?** Send rate per burn on a
good day, Roadside's open projects:

| project | true | workable, no beta | clean + wired, skill 65 | skill 75 |
|---|---|---|---|---|
| the slab right of the pull-off | V3 | 99.8% | 100% | 100% |
| the short wall behind the cattle grid | V4 | 86.2% | 100% | 100% |
| the arete left of Diesel | V8 | **0.0%** | **66.8%** | 96.8% |
| the low traverse into Chalk Ghost | V8 | 0.0% | 40.0% | 95.0% |
| the blank wall behind the parking | V10 | 0.0% | 0.0% | 1.5% |

**The whole difference between impossible and two-thirds is cleaning it
properly and learning it.** The probe brushed every line to exactly
`workable` (0.55, the minimum) and never rehearsed, so it saw 0% and
concluded the valley was used up. Same shape as the crash pad: nobody had
ever tried.

### Wrong turn one: a `projector` policy barely helped

Brushing to 0.98 instead of 0.55 moved almost nothing, because the climbers
were not strong enough for the V8s either way. So it was not dirt.

### Wrong turn two: I misread a column and nearly reported a collapse

The next measurement appeared to show careers ending at **V0.35** — a
climber who could not do a V1 at 54 having started at V5 — which would have
been a serious bug in the aging model. It was field 34 of the probe's row,
which is `shoewear`. `allround` is field 33.

Read correctly, a career ends at **V4.0–V5.0**, and the shape is exactly the
designed one:

| | power | fingers | technique | endurance | head |
|---|---|---|---|---|---|
| start (24) | 50 | 50 | 50 | 50 | 50 |
| end (54) | **16.8** | 42.2 | **59.3** | 40.6 | **55.8** |

Power collapses, fingers and endurance slide, **technique and head are
higher than they started**. "You climb smarter than you pull now", in
numbers. No bug.

### So: it reads like a valley

A career starts at V5.0, peaks around **V7**, ends around V4.5. The V8
projects need skill ~65 clean and wired — **the very top of a peak career**,
reachable in the strong years and not otherwise. The V10 needs 75+, which
nobody in this world ever reaches.

Over ninety years and four generations: **the player puts up one, the Lot
puts up three, and one line is still standing at the end.** That is not an
exhausted valley. That is a valley where the plums go to whoever is
strongest, and the testpiece stays unclimbed.

**What is left is a question of taste, not a defect:** the player gets one
of the four that go, because Dev is stronger than any career the player
will have. Whether that is right is Evan's call. It is certainly true to
life, and it may be thin for a game.

## 3. What that leaves, which is a content problem and not a bug

Generations 2, 3 and 4 put up **no first ascents at all**. Roadside has five
projects; generation one takes the V3 and the V5, and nobody after them ever
does another, because the remaining three are V7–V9 *and* filthy, and a
probe climber peaks around V6–V8.

So the ninety-year guidebook reads like **one person lived there and three
more failed**. The gate says it should read like somebody lived there; it
currently reads like somebody *arrived*, and then the valley ran out.

Not fixed here, because it is a content decision rather than a defect, and
the options are real ones:

- **More projects**, or projects that regenerate — a valley with five
  unclimbed lines is exhausted in one career.
- ~~**The Lot should put up lines too**~~ — **done, and it found a third
  bug.** See below.
- **Accept it**, and make a career the unit of play rather than a dynasty —
  in which case the gate should say so.

## 4. The Lot's ascents never reached the book either

`PartnerTakesFirstAscent` has recorded claims since the Lot was built — into
the partner's own list, and nowhere else. The crag never learned. So a line
Dev put up last spring stayed an open project with nobody's name on it, and
**the player could still walk up and take its first ascent.** Ninety years:
the Lot took five lines and the guidebook showed none of them.

`TheyPutUpTheLine` names it in their voice, signs it, and takes it out of
the projects. Keyed on the *name* rather than the Partner, because partners
are rebuilt from the world seed daily and only `PartnerBond` is saved — so
replaying the bonds' route keys is what puts the Lot's ascents back on the
page after a reload, the same shape as the legacy replay above.

### And then the Lot took everything

With the claims actually landing, ninety years gave **the player zero first
ascents and the Lot all five.** That was not the wiring over-correcting. It
is what the Lot has always done; the book ignoring it was the only reason
the player ever got one.

The dial is `firstAscentChancePerDay = 0.02`, and its comment says *"the
player should usually get the chance if they commit"*. Measured across six
thirty-year careers:

| chance/day | player's lines | the Lot's |
|---|---|---|
| 0.02 (shipped) | **0** | 30 |
| 0.01 | 0 | 30 |
| 0.005 | 0 | 30 |
| 0.002 | 1 | 29 |
| 0.0005 | 3 | 27 |

**A per-day roll over a thirty-year career converges on certainty however
small it is.** At one in two thousand the Lot still took 27 of 30. The
dial's stated intent is not reachable by making the number smaller.

It is reachable by making commitment mean something. Nobody at the Lot takes
a line somebody is visibly working — that etiquette is real, and it is the
only version where a project is worth committing to. `SpokenFor` covers
anything you have pulled on, brushed, or already put up.

| | player's lines | the Lot's |
|---|---|---|
| 0.02 | 2 | 15 |
| 0.01 | 2 | 15 |
| 0.005 | 2 | 15 |

**Identical at every dial value**, which is the sign it is a rule rather
than a tuning. You keep what you commit to and lose what you ignore.

> A dial got a name out of this too. The brush threshold started as a bare
> `> 0.2` and a test caught that `ProjectMemory::cleanliness` defaults to
> **1.0** — an established line is clean — so the rule called every route in
> the book "spoken for" when asked about one. It is only ever asked about
> projects, so the shipped behaviour was right and the rule was fragile.
> `brushedEnoughToBeYours` says which it is.

## Re-measured, 2026-08-24 — and the probe was the thing that was wrong

Re-run with the argument collision fixed (`notes/the-arguments-that-meant-
two-things.md`), the numbers above did not reproduce. **Ninety years gave 4
to 88 lives depending on the seed**, mean 17.4, against this note's four.

The confound was not the cause: forcing `build=1` — the archetype the broken
arg 6 had been setting — changes nothing, and on two seeds makes it worse.
Something else had moved.

### 88 careers, 68 of them retired at 24

Instrumenting which of `TimeToThinkAboutIt`'s two triggers fired made it
obvious. On the worst seed, **87 of 87 endings were the body**, at a mean
retirement age of **24.7** — a dynasty of people who inherited the van,
climbed for a year, got hurt three times and quit. The grade trigger, the
one this phase's design is actually about, fired **zero** times.

Two causes, one of them a parity bug and one of them the probe modelling a
person badly.

**The probe and the engine disagreed about the counter they both feed.**
`TimeToThinkAboutIt` takes `consecutiveInjuries` as an argument, and the two
consumers built it differently:

| | probe | engine |
|---|---|---|
| clean stretch that resets it | **120 days** | **90 days** |
| a day counts toward that if | not hurt **and** not hurt yesterday | not hurt |

So the measured career was offered the door on a stricter rule than the
played one. **`check-parity.py` cannot see this** — both consumers call the
rule, and the divergence is in what they hand it. *"It catches the probe
running a rule the game never runs; it cannot catch the game running the
rule and disagreeing about the input"* is that checker's own stated blind
spot, and this is it. The probe now matches the engine exactly.

**And the probe always said yes.** The justification in the code was *"a
probe that declines would measure nothing"* — but a probe that accepts at
twenty-four measures nothing either, and worse, it looks like data.
Retirement is an offer and never a command (`LegacyDials`), and nobody
quits at twenty-four over one bad year. There is now a `retire=` policy:
`always` (the default, so every earlier measurement still reproduces) and
`late`, which takes the grade offer — it already requires forty-six — and
declines the body's until then.

### With a person answering the question, this note is right

Ninety years, 10 seeds:

| | always | **late** | this note claimed |
|---|---|---|---|
| lives | 17.4 (4–88) | **3.3 (3–4)** | 4 |
| retired at | 36.5 | **55.1** | 46–52 |
| ended by the body | most | **none** | — |
| peak | V7.03 | **V7.61** | ~V7 |
| lines: player / Lot | 0.5 / 3.1 | 0.5 / 2.7 | 1 / 3 |

**Every structural claim in this note survives.** A career runs to the far
side of fifty, ninety years is three or four lives, a career peaks around
V7, and the valley ends with the player having put up about one line and the
Lot the rest. What had rotted was the probe, not the finding — and it rotted
because Phase 10 gave the game a body that actually breaks, three phases
after this note was written.

### The one that is a design call, not a bug

A `late` climber **declines the offer 856 times in ninety years** — about
nine and a half times a year, every year, from twenty-four. The body trigger
is not rare and it is not quiet. In the played game that is the one opinion
the game ever offers about your career, arriving roughly monthly for thirty
years before it ever means anything.

Whether `injuriesInARowToHint = 3` over a 90-day window is the right shape
is Evan's call. It is doing exactly what it says; the question is whether
what it says should have an age floor under it, the way the grade trigger
does.

### And the commitment rule holds

`SpokenFor` re-tested across the Lot dial, 6 seeds, ninety years:

| `lot=` | player's lines | the Lot's |
|---|---|---|
| 0.02 | **0.67** | 2.83 |
| 0.01 | **0.67** | 2.33 |
| 0.005 | **0.67** | 2.17 |
| 0.002 | **0.67** | 2.17 |

The player's column is **identical at every dial value**, which is the half
the claim is about: *you keep what you commit to*. The Lot's column moves
with the dial, which is the dial doing its job — this note's older table had
both columns frozen, and that was the 30-year scale saturating rather than a
property of the rule.

## What the probe now supports

`build/season <days> <seed> <rest> v greedy careers=1` runs
multi-generation and prints the guidebook with route keys.

**The invocation above used to read `... v greedy 1.5 -1 keep -1 careers`,
and that form is now refused.** It did not mean what it looks like: arg 6
was read as the skin regen *and* as the character build, so `1.5` built
archetype 1 and overwrote the starting cash — every number below was
measured on that climber rather than on the default one. See
`notes/the-arguments-that-meant-two-things.md`. Ninety years is
four lives; thirty is barely one, because a career runs about 29 seasons and
retires at 46–52.

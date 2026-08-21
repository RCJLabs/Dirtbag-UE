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

## What the probe now supports

`build/season <days> <seed> <rest> v greedy 1.5 -1 keep -1 careers` runs
multi-generation and prints the guidebook with route keys. Ninety years is
four lives; thirty is barely one, because a career runs about 29 seasons and
retires at 46–52.

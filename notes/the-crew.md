# What the town calls you

**2026-08-21.** Second of the four unported systems. Pillar 6 of
`concepts/DIRTBAG.md`: *"a crew the town names without asking you."*

**That last clause is the whole system.** You do not pick it, you cannot
change it, and it does not come back for revision. Everything else follows.

## The rule

- **Two people you actually climb with.** You plus one is a partnership,
  which the game already models as a bond and does not need a name for.
- **Rapport 0.5** is what counts as climbing with somebody rather than
  knowing their name — roughly a fortnight of turning up, against a decay of
  0.01 a day.
- **Thirty days of it.** The point of a nickname is that somebody said it
  twice; a crew that exists for one good week in September is three people
  who had a good week in September.
- **A thin fortnight does not cost you the month.** The counter slides back
  at the rate it built rather than resetting, so a crew where somebody was
  hurt or working still gets there.
- **Named is named.** It does not revise when the crew drifts apart. A
  career that outlives its own crew still gets called the thing it got
  called.

## Where the name comes from

The faction you stand highest with, because that is the part of town doing
the talking — and standing is earned by what you actually did, so the name
is downstream of behaviour rather than a label anybody chose.

Then a hash of the world seed and the members' names, **sorted**, so the same
people in the same valley are always called the same thing. Sorting matters
more than it looks: partners are rebuilt from the world seed daily and only
the bond is career state, so an unsorted key would rename you on reload.
There is a test.

Four lists plus one. The fifth is what you get when nobody rates you —
*those kids*, *the van in the corner*, *whoever they are*. Still a name,
because **being called something is the point**, and a valley that has not
decided about you has still noticed the van.

## It came out emergent, which was the hope

Four worlds, first two lives each, from the career line:

| | |
|---|---|
| crag-1, 23 seasons, 2 first ascents | **the brush crew** |
| crag-1, 8 seasons | **the Lot lot** |
| crag-2, 27 seasons, 2 FAs | **the ones with the drill** |
| crag-2, 4 seasons | **the loud corner** |
| crag-3, 28 seasons, 2 FAs | **the brush crew** |
| crag-4, 23 seasons, 1 FA | **the new-route lot** |
| crag-4, 9 seasons | **the campfire crowd** |

Nothing scripted this. The long careers run the `stakeout` policy, which
brushes constantly to claim lines — that earns development standing, and the
development crew calls people *the brush crew* and *the ones with the drill*.
The short social lives never build it and the scene names them instead.

Reading a career line now:

> 27 seasons. Hardest: The Guidebook Lied, V7. 2 lines that are yours now.
> **They called them the ones with the drill.** 26 Dirtbag Years. the short
> wall behind the cattle grid never went, after 46 tries. Retired at 50, with
> the crag open.

## Where it surfaces

- `CrewLine()` — with the slow numbers. The slowest fact on the screen: said
  once in a career, never revised.
- `CrewNews` — the morning it lands. *"You heard somebody at the fire call
  you the trail crew."* Not "you are now called" — **nobody announces a
  nickname to your face**, you hear it secondhand, which is how they arrive.
  Drawn above the Dirtbag Year line rather than sharing it, because a career
  can do both in one night and the rarer one must not be overwritten.
- `LegacyText` — before the years and the nemesis. Who they were, before
  what they did.

## Save

`kSaveVersion` 16 → 17, `MigrateV16ToV17`. An old save arrives **unnamed**,
which is the true answer rather than a lossy one: the town had not said it
because the system did not exist. The bonds that earn one are already saved,
so an existing career starts its month from today and gets named on the far
side of it, exactly as a new one would.

One trap worth naming: an unnamed crew has to round-trip as
*present-and-empty*, or a fresh save would look like an unmigrated one on the
next load. `line.substr(eq + 1)` yields `""` for `crew.name=`, so the key is
parsed as required rather than optional. Tested both ways.

## Three places, not one

Adding a sim file needs three edits and the harness only catches one of them:
`Sim/run-tests.sh`'s file list, a `Source/DirtbagUE/SimCrew.cpp` bridge
because UBT only compiles files under the module, and
**`DirtbagSimTypes.h`'s include aggregate** — the one that was missing for
`DirtbagSport.h` and cost a build cycle. All three done here, and preflight's
"24 sim files bridged" is what confirms it.

## What might still need the 2D game

The rule is my reading of five words, same as the Dirtbag Year. If the 2D
version names you after something other than faction standing — your hardest
grade, the van, one specific partner — that is a change to `CrewDay`'s name
selection and nothing else. The thresholds are dials.

The one thing I would not change without a reason: **you do not get to pick
it.** That is not a threshold, it is the pillar.

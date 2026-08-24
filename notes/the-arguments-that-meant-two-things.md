# The arguments that meant two things

*2026-08-24*

The job was a re-measurement. The fixed day (`notes/the-day-that-never-
happened.md`) moved the food economy, so every balance conclusion taken on
the old probe needed re-testing against the new one — a paired before/after
on identical seeds, using a git worktree at the pre-fix commit so both
binaries exist at once.

It found the day fix was the smaller of the two problems.

## The probe's arguments collided, and had for months

`Sim/tools/season.cpp` took its options positionally. Two of the slots had
grown a **second reader each**, a hundred lines apart, both live, both
documented in a comment beginning "Arg 6" / "Arg 7":

| slot | one reader | the other |
|---|---|---|
| arg 6 | `skinRegen` | `buildSpec` — archetype/origin/flaw/temperament |
| arg 7 | `startingPads` | `withRival` |

Neither knew about the other, so:

- **Every skin-regen sweep also set the character build.** `atoi("1.5")` is
  1, `atoi("2.0")` is 2, `atoi("9.0")` is 9 — so each row of the sweep was a
  different archetype, and `buildSpec` being non-empty also replaces
  `player.cash` with that character's starting cash. The sweep in
  `notes/phase3-what-binds.md` is six rows that differ in three things at
  once.
- **Every `rival` run set the pads to zero.** `atoi("rival")` is 0, and arg
  7 is the pads. So Phase 8's rival career was measured on the padless
  season — the exact knob whose own comment says *"0 pads and 2 pads are the
  bold and the safe season, and the gap between them is the whole
  mechanic."*
- **And my own `-1` "no-op sentinel" was undefined behaviour.**
  `buildSpec = "-1"` gives `part[0] = -1` and then
  `static_cast<Archetype>(-1 % 4)`, which is an out-of-range enum cast. I
  wrote it while trying to *hold skin regen at its default* and it silently
  reconfigured the climber instead. That is how I found this: two runs of
  what should have been the same measurement disagreed by a factor of two,
  and the difference was the sentinel.

**None of the thirteen checkers could see it**, and that is fair — they
check the sim and the engine, and this is the probe's own front door. The
tell was available the whole time and nobody read it: the two comments both
say "Arg 6".

### Fixed by making it impossible

Everything past the policy is now `key=value` — `skin=` `pads=` `build=`
`rival=` `norubber=` `foam=` `careers=` `lot=` `savings=` `med=`. A repeated
key is refused, an unknown key is refused, and a bare token is refused with
the list of what is known. A probe that shrugs at `pads2=1` and quietly
measures the default is a probe that lies, so all three are hard errors
rather than warnings.

The old positional form is now rejected loudly, which is the point. Two
documented invocations were stale and are updated.

## What survived, measured

Paired, identical seeds, before and after the day fix.

### The Phase 3 gate: **survives, and got stronger**

`kept` (no bills, no food, no fuel, never works a day) against `greedy`, 12
seeds, one season:

| | days | sends | burns |
|---|---|---|---|
| before | **+5.4** | −0.9 | +23 |
| after | **+0.4** | +0.3 | −12 |

Deleting the entire cost of being alive bought five climbing days before and
buys **half a day** now. The send difference is ties in 6–7 of 12 seeds
either way — noise, as it always was. *Money does not buy quantity* is the
restated gate and it holds harder than when it was written.

### What the day fix actually cost: **about nine days of work a year**

Same policy, before → after, 12 seeds:

| | before | after | seeds moved |
|---|---|---|---|
| work% | 18.7 | **21.2** | up in **11 of 12** |
| days climbed | 106.4 | 111.4 | 5 up / 5 down |
| sends | 3.2 | 2.1 | **7 of 12 identical** |
| injuries | 1.7 | 1.2 | 11 of 12 identical |

The only clean signal is the work dial. Everything else is inside the noise
— and the apparent 34% send drop is **not real**: seven seeds are identical,
and widening to 18 seeds flips its sign. A single season's send count is far
too noisy to carry a conclusion, which is worth remembering next time one
does.

### "Skin was never the wall": **survives, and the clean number is much stronger**

The sweep, re-run with the build held fixed — which is the first time it has
been run without the archetype changing underneath it:

| regen | burns | days out | injuries | days hurt | all-round |
|---|---|---|---|---|---|
| **1.5** (shipped) | 557 | **111** | **1.2** | **46** | 5.59 |
| 3.0 | 574 | 99 | 4.0 | 164 | 5.54 |
| **9.0** | **592** | **88** | **3.6** | **159** | 5.57 |

Six times the skin budget buys **6% more burns** — the confounded table said
54% — and costs three times the injuries, 113 more days hurt, and 21% of the
climbing days. **All-round grade is flat to two decimal places across the
entire sweep**: six times the skin produces the same climber. The
conclusion was right and was being *understated* by its own measurement.

### Phase 8's rival: the rival is fine, the climber was not

5-year careers, 12 seeds:

| | sends | head | lines lost to them | their generations |
|---|---|---|---|---|
| as Phase 8 measured it (padless) | 15.2 | **64.0** | 0.5 | 0 |
| with the pads back | 20.5 | **57.2** | 0.5 | 0 |

The rival's own numbers are **identical** — same lines taken, same
generations — so what Phase 8 says about the rival stands. What is distorted
is the climber it was said about: head inflated 12%, sends deflated 26%.
Any Phase 8 claim about *head* or *sends under a rival* is measuring a
bolder climber than the shipped default.

## What is still open

`notes/phase4-career.md` ran multi-generation with the same broken form, so
its numbers were taken on archetype 1 with that character's starting cash
rather than the default. Its structural claims (a career runs ~29 seasons,
retires at 46–52, ninety years is four lives) are about the legacy system
rather than about the build and are probably safe; the grades and money in
it are not. Re-running it is the next job.

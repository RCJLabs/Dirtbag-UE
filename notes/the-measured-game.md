# The measured game and the played game

*2026-08-23. Written after the fifth no-door system, when the pattern
underneath all of them became clear enough to check for.*

## The pattern

Three findings in two days, all the same shape:

| rule | the probe | the engine |
|---|---|---|
| `WorkOddJob` | took gigs off the board, with their standing effects | called `WorkShift` — a flat wage, **no standing effects at all** |
| `GoToTheGym` | checked `IsGymMember` | reached the gym by travel spot and wall, **neither of which asked** |
| `ObligationToday` | took the sponsor's days | **asked nobody** |

**A rule living in a sim function the engine never called, with a
reachable path around the side.**

The consequence is worse than a missing feature. Every balance note in this
project comes from `Sim/tools/season.cpp`, so when the probe exercises a
rule the engine skips, **the career being measured is not the career being
played** — and the numbers do not describe the game. Work and reputation
were joined in every measured career and disconnected in every played one.
The gym cost $75 a month in the notes and $0 in the build.

None of the ten existing checkers could see it. They check that
declarations are *reachable*; this is about whether the sim's two consumers
**agree on which rules apply**.

## The check

`tools/check-parity.py`, the eleventh. For every rule the probe calls, is it
reachable from the engine? Escape with `// probe-only: <why>`.

Three are marked, and all three are honest:

- **`GoToTheGym`** — the probe's one-call shortcut for an indoor day. The
  rule inside it is enforced in the played game now, at the wall, which is
  what makes it a shortcut rather than a divergence. It was a real
  divergence until today.
- **`OpenProjects`** — the guidebook screen filters the mirrored crag it
  already holds rather than converting back to sim types to ask. Same
  answer, no balance rule in it.
- **`FactionName`** — a formatter for the probe's own report.

**Zero divergences.** For the first time in this project, the measured game
and the played game are provably the same game.

## Getting it right took three tries, and two of them passed while blind

**It saw 33 rules out of 92.** The declaration regex excluded `{` to avoid
running into function bodies — which made it blind to every declaration
with a braced default argument, `const DayDials& dials = DayDials{}`, which
is *most of this sim*. It reported zero problems, and all three divergences
it was written for were in the part it could not see. **A checker that
passes because it is not looking is worse than no checker.**

**Then it flagged rules that were fine.** `FactionDay` and `KitDay` are
called inside `SleepToNextDay`, which the engine calls every night —
standing drift and membership expiry were working exactly as designed, and
counting only *direct* engine calls called them divergences.

**Then it excused one that was not.** Counting any call from anywhere in
the sim as reach was too generous by exactly one case: it excused rules
called only from `GoToTheGym`, which is itself only ever called by the
probe. A rule reached only from a dead function is not reached.

It does proper reachability now — seed with what the engine calls directly,
add what the reached rules call, repeat until nothing new appears.

## What it cannot see

It catches *the probe runs a rule the game never runs*. It cannot catch
*the game runs the rule and ignores the answer*.

That distinction has a real example in this very finding. `IsGymMember` was
**always** reached — `KitLine` calls it, so the HUD cheerfully printed
*"gym membership: 12 days left"* the whole time nothing enforced it. The
gate was what was missing, and the gate is `GoToTheGym`, which is what the
checker flags. It found the right thing under a different name, which is
luck as much as design.

A check for *computed and ignored* may not be greppable at all.

## Why this was the right next thing

The recommendation before this was a career-scale re-measurement, and it
was **wrong** — I had claimed the probe never signed a sponsor and never
saw a physio. It does both, and it runs `ReviewSeason` too. The sim economy
did not change while the doors were being built, so re-running it would
have reproduced numbers that had not moved.

The useful question was never *what do the numbers say now*. It was
**whether the numbers were ever about this game** — and now they provably
are, with a check that keeps them that way.

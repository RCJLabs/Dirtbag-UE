# What never happens

*2026-08-24*

Nearly every real bug this project has found late was **a zero in a column
nobody read**:

| | |
|---|---|
| `nobelayer` = 0 | the belayer gate had never bitten in ninety years |
| `starved` = 0 | *"the dirtbag's oldest trap"* had never fired, ever |
| `gaway` = 0 | nobody was ever away, because nobody ever left town |
| sport = 0 careers | the Cave had never been climbed |

Each was sitting in the probe's own output for weeks. Each took a person
noticing. `tools/never-happened.py` asks the question on purpose: run the
probe across a battery of policies and option knobs, and report every
counter that came out **zero in every single run**.

**It is the mirror of `check-parity.py`.** That one catches *the probe runs
a rule the game never runs*. This one catches *the game has a rule and
nothing ever reaches it* — which is the direction that hid the Cave, the
belayer, the trap and the tooth's dead end.

## The tool's own coverage was the bug, twice

Worth writing down because it is the failure mode of every coverage tool,
and a coverage tool whose gaps are its own is worse than none — its output
reads like a bug list.

**First run**: 27 dead counters, and most of them were dead because nothing
in the battery had a rival, saw a physio, or entered a race. The battery was
policies only; the probe's interesting behaviour lives as much in its option
knobs. Adding `rival=`, `med=`, `careers=` and `pads=0` took it to 15.

**Second run**: it reported the entire top of the competition ladder dead —
the national team, the World Cup, the Games. It is not dead. Team selection
needs a ranking of 700 and a career gets there in its twenties; I had run
ten-year careers. **A short run's output is a list of things that have not
happened *yet*, which is a different question and a worse one.** The default
is now thirty years, which is what this game is.

## What is actually left, at thirty years

Thirteen counters, and they split cleanly in two.

### The probe has never taken these actions

Four verbs the engine offers and **no measured career has ever used**:

| | |
|---|---|
| `prehab` | `DoPrehab` — twenty minutes of a morning |
| `meds` | `TakeSomethingForIt` |
| `toothfixes` | the dentist |
| `surgeries` | `HaveSurgery` |

So **every balance claim this repo has made about the medical system was
measured on a climber who never takes a pill, never does prehab, never gets
a tooth seen to and never has surgery.** The whole treatment half of Phase
10 is unmeasured — not broken, *unmeasured*, which is the state the Cave was
in before somebody sent the probe there and it came back with four findings.

`check-parity.py` cannot see this, and correctly: it asks whether the probe
runs rules the game does not. Nothing asked the reverse until now.

### And the ones that are about the game

- **`wcpods`, `wcwins`, `wctitles`, `games`, `medals`.** A career now gets
  World Cup *starts* — that moved once the run was long enough — but has
  never once podiumed, and has never reached the Games. The ladder's top
  three rungs are reachable in principle and unreached in fact.
- **`gwon`** is downstream of that: no season title, so no local ever says
  *"somebody said you won the season."*
- **`sackings`** — nobody has ever lost a job.
- **`ranout`** — a full rack has never emptied in a played career. This one
  is a **known** item, already recorded, and the tool rediscovering it
  independently is a decent argument that the tool works.
- **`gaway`** — nobody is ever away, which is now *correct*: since the day
  fix a career eats in town three days in four. The line exists for a
  player who leaves the valley, and the probe never does.

## How to read the output

None of these is automatically a bug. A rule that never applies may want a
policy that provokes it, or may be waiting for a part of the game that is
not built. **What none of them should be is unexplained** — every entry on
that list is worth a reason written down, and the four medical verbs are
worth a probe policy first.

Not part of `preflight.sh`: it takes minutes rather than seconds, and its
output is a reading list rather than a pass or a fail. Run it when a phase
lands, which is when a system is most likely to have arrived with nothing
reaching it.

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

## The tool's own coverage was the bug, three times

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

**Third run — and this one got into a committed note before I caught it.**
It reported `prehab`, `meds`, `toothfixes` and `sackings` dead, and I wrote
that those were *"four verbs the engine offers and no measured career has
ever used"* and that the treatment half of Phase 10 was unmeasured. **That
was wrong.** `med=` composes: `-up` turns on prehab, meds and the tooth, and
`-hard` answers every shift the hard way, which is the only route to being
sacked. Both flags' own comments in `season.cpp` say in as many words that
they exist so those counters can fire — `-hard`'s says a policy without it
*"never botches, never loses standing and is never sacked, so the sacking is
testable in the harness and unreachable in a played career, which is the
same as not existing."* My battery had `med=careful-ins` and neither suffix.

With the suffixes in, all four fire. The lesson is now in the tool's own
docstring: **a "never happened" is a claim about the battery until you have
checked it against the probe's own flags.**

## What is actually left, at thirty years

Nine counters, across 29 configurations × 3 seeds.

### The one that is a real balance finding: nobody can afford to be treated

`surgeries` stays at **zero even for `med=careful-up`** — the policy whose
own comment calls it *"the only policy that uses the expensive end of the
medical system, and therefore the only one against which insurance can be a
bet at all."* It scans every injury and never once operates.

The reason is not the gate. It is the price:

| | costs | a thirty-year career's **richest single moment** |
|---|---|---|
| a guess | $60 | |
| cortisone | $180 | |
| a scan | $340 | **$450 – $518** |
| **surgery** | **$2,200** | |

**Surgery costs four to five times more than the career ever holds**, at its
best day, in thirty years. A scan at $340 is two thirds of that peak, which
is why an uninsured careful climber manages **0.3 diagnoses in thirty
years** — even looking is barely affordable.

And the consequence for the thing built on top of it: insured, the same
career pays **$6,256 in premiums and gets $544 back.** The premium total was
known — `MedicalDials` says so in a comment, *"thirty years of premiums at
this number is about $6,250, measured"* — but the claims side was not, and
it cannot pay, because the treatments it would cover are unaffordable with
or without it. **Insurance is not a bet in this game. It is an eleven-to-one
losing one.**

None of this is a bug in the medical code, which does exactly what it says.
It is a price list written against a wallet this game does not have: Phase
3 measured that 98% of everything earned goes straight back out and a career
ends on about $200. **Every price above roughly $300 is theoretical.**
Whether that is right — a dirtbag genuinely cannot afford surgery, and that
is the most honest sentence in the game — or whether the expensive end wants
repricing so the choice exists at all, is a design call and it is Evan's.

### And the ones that are about the game

- **`wcpods`, `wcwins`, `wctitles`, `games`, `medals`.** A career now gets
  World Cup *starts* — that moved once the run was long enough — but has
  never once podiumed, and has never reached the Games. The ladder's top
  three rungs are reachable in principle and unreached in fact.
- **`gwon`** is downstream of that: no season title, so no local ever says
  *"somebody said you won the season."*
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

# What the 2D game still has that this one does not

**2026-08-21.** Evan: *"What else? I know the game isn't fully built yet."*

Right — so I went to the spec rather than guessing. `concepts/DIRTBAG.md` §4
lists what was meant to be **ported wholesale**, because the design was
already proven:

> factions (4, opposed), persistent partners with their own careers, the Lot
> neighbors and campfire, van as wearing machine (6 components,
> patch-vs-replace), gear brands/lean/wear, odd-jobs board, salaried-job trap
> **+ Dirtbag Year achievement**, **dreams (Rig/War Chest/Home Base)**,
> projecting/nemesis tracking, injury + physio + aging, legacy/generations,
> the dog, **the crew-naming system**, ethics arcs.

Checked every item against `Sim/`. Fifteen of the nineteen are built and
measured. **Four were never ported**, and nothing in the repo records a
decision to drop them.

> **Corrected 2026-08-23 — read `notes/the-2d-audit.md` instead.**
> Everything below is accurate about §4's nineteen-item list. It is badly
> wrong about *"what the 2D game still has"*, which is the title and the
> claim. §4 is a paragraph written from memory; the real game is 48,447
> lines with a 497-field save and about ninety systems. Auditing against
> the summary and reporting it as the game is the same error as testing an
> ordering when the question was a magnitude — the check passes and tells
> you nothing. The four gaps named here are real and are now built. They
> were never the whole gap.

| missing | evidence | needs the editor? |
|---|---|---|
| ~~**Dreams** (Rig / War Chest / Home Base)~~ | **built 2026-08-22** — `notes/dreams.md` | no |
| ~~**The crew-naming system**~~ | **built 2026-08-21** — `notes/the-crew.md` | no |
| ~~**Dirtbag Year achievement**~~ | **built 2026-08-21** — `notes/dirtbag-year.md` | no |
| ~~**The campfire game**~~ | **built 2026-08-22** — `notes/the-campfire-game.md`; rules are sim-side, only the table UI needs the editor | rules no, UI yes |

Also absent and *deliberately* so — §4's cut list: comps, expeditions, deep
water solo, big wall, filmmaking, gym ownership, Solo mode. Those are
recorded decisions and are not gaps.

## Dreams is the one the measurements have been asking for

A thirty-year career on the `saver` policy — the one that actively tries to
accumulate:

| | |
|---|---|
| earned | **$223,842** across 2,292 shifts |
| **highest cash ever held** | **$447** |
| bills | $132,940 |
| fuel | $46,152 |
| dog | $21,891 |
| van | $6,965 |
| kit | $4,470 |
| shoes | $3,850 |
| food | $3,472 |

**Ninety-eight percent of everything earned goes straight back out**, and the
balance never once clears $450 in thirty years. Over ninety years and four
generations it is the same shape: $640,665 earned, cash high $417.

There is nothing to save *for*. Kit is the only durable purchase and it caps
out in the first season — after that money is a treadmill, and the only
question it ever answers is whether you can afford this month.

This is the thing Phase 3's gate kept circling. The gate was restated to
*"money decides how good your burns are, and deciding is something the
player actually does"* — and it passes, narrowly, on kit. But the 2D game
had a second answer that this build never got: **money decides what you are
working towards.** That is what a dream is, and it is why the 2D game can
have a salaried-job trap that means something. A trap needs somewhere you
were trying to get.

## What I cannot do from here

**`concepts/DIRTBAG.md` names dreams once and never defines them.** Three
words — Rig, War Chest, Home Base — and no mechanics anywhere in this repo.

CLAUDE.md is explicit that the 2D game is the spec and that deviations are
pivots to be logged rather than improvised. Designing a dream system from
three words would be improvising a named system that already exists and
works, which is the one thing the house rules say not to do.

So this needs Evan, and specifically:

1. **What does each dream cost, and what does it change?** Rig sounds like a
   better van; War Chest like a cash buffer that buys something — time? a
   season off? Home Base like the end of living in the van.
2. **Do you pick one, or do they sequence?** A single chosen dream is a much
   stronger design than three checkboxes, and it would make the salaried-job
   trap bite properly: the fastest way to a dream is the job that costs you
   the climbing.
3. **What happens when you get there?** Does the run end, does the goal
   change, or does the dream become upkeep?

Answer those and the system is a day's work in `Sim/`, engine-free and
tested, with no editor needed until it reaches the HUD.

## The other three, ranked

**The Dirtbag Year achievement** is the smallest and needs almost nothing
from the 2D game: `Jobs::weeksSalaried` already tracks the exposure, so the
rule is presumably "a full year without taking the salaried job". Worth
confirming the 2D wording, buildable in an hour, and it gives the
salaried-job trap the counterweight it currently lacks.

**The crew-naming system** — pillar 6, *"a crew the town names without asking
you"* — has all its inputs already built: four factions with standing, named
partners with bonds, a reputation model. It is the cheapest way to make the
scene feel like it is watching, and it is pure sim. Needs the 2D game's
naming rules.

**The campfire game** is the only one that genuinely waits for the desk. §4
says keep exactly one, and which one is a taste call.

## Not a gap, worth saying

Fifteen of nineteen ported, all of them measured rather than assumed, plus
sport and the runout, three crags, a head model, shoe wear, crash pads and a
first-ascent pipeline the 2D game did not have in this form. The four above
are what is left of the port list, not the state of the project.

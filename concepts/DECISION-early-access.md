# Decision: Early Access — yes, and not for a long time

*2026-08-22. Evan's call, recorded because `concepts/` is where decisions
live and this one had been scheduled for two years without ever being made.*

## The decision

> *"we will do ea… so yes to ea. but way in the future. first we need to
> continue building what we can."*

**Early Access is the shipping plan.** It is not near, it is not next, and
nothing in the roadmap should be sequenced as though it were.

## What was actually decided before this

Nothing. Two one-line scheduling markers, written at planning time:

- `DIRTBAG.md` §10 — *"M3+: rungs 2–4, Early Access candidate at rung 2."*
- `DIRTBAG-PLAN.md` §Phase 4 — *"Early Access decision at (a)."*

That is the whole prior specification: a marker saying *decide around here*,
carrying no content. The build passed rung 2 some time ago and is at rung 4,
so the marker had been sailed past twice without anybody noticing it was a
question.

## Why "yes" is the easy half

The systems are two rungs beyond where EA was planned to be considered, the
cut ladder is complete except trad, saves migrate cleanly from v1, and the
market argument in `DIRTBAG-PLAN.md` §1 has not changed — *"No serious 3D
climbing life-sim has ever shipped… the Steam tag is starving."*

## Why "not for a long time" is the correct half

Evan, naming it plainly and more usefully than any measurement here could:

> *"wow we dont even have anything built. just testing spots on the
> playground template in unreal. so a lot of work to do. i dont even know
> what assets to use or how to put things together."*

**That is the true state and it should be written down in exactly those
terms.** Twenty-six sim modules, a career that runs headless for ninety
years, ethics and sponsorship and legacy and three campfire games — all of
it currently played on Epic's playground template with trigger volumes
standing in for a valley.

The systems are not the project's remaining work. **The place is**, and none
of it exists.

Three specific things are also still true:

- **Nobody but Evan has ever played it.** Phase 5's own gate asks for
  exactly that and has not been run.
- **Three play gates are unverified** (Phases 1, 2, 4).
- **`DIRTBAG.md` §11's four questions have no recorded answer anywhere** —
  first vs third person, stylized vs realistic, whether the 2D canon carries
  over, PC-first. The first two are decided the moment a build exists, and
  they have not been decided.

## What this changes

**Nothing about the order of work, which is the point.** EA stops being an
item that gates anything and becomes the destination. Concretely:

1. **Phase 5 item 5 is closed by this document.** The decision is made; it
   is no longer waiting on the phase.
2. **No date, no price, no store page, no scope promise.** Every one of
   those is a public commitment and none is answerable from a playground
   template. They get decided when there is a place to stand in.
3. **The two §11 questions that a build decides by existing** — camera
   person and art direction — become the *next* real decision, because
   assets cannot be chosen without them. That is a gate on the world, not
   on EA.
4. **The 2D game stays live as the full experience**, per `DIRTBAG.md` §9's
   own mitigation. Nothing here competes with it.

## The honest read of where the work went

The container built systems because systems are what a container can build
and verify: engine-free C++, a g++ harness, nine preflight checkers, golden
vectors. That work is real and it is done to a standard.

It also means the project accumulated its depth in the one dimension that
was never the risk. `DIRTBAG.md` §9 named the top risk correctly — *"Session
isn't tense/legible in 3D"* — and §7 named the craft centre correctly —
*"session staging: camera, minigame UI, animation state machine."* Phase 5
is the first work aimed at either.

The next bottleneck is not another system. It is a valley, a lot, a van and
a town, and knowing which assets make them.

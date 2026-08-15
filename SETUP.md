# SETUP — creating the Unreal project around the sim core

The container this repo is developed in has no Unreal Editor, so the sim core
(`Sim/`) is built and tested standalone (`Sim/run-tests.sh`, plain g++). These
are the steps only you can do, on your machine. The landnam-ue project is the
template for most of this — same method, second verse.

## 1. Create the project

1. Unreal Engine **5.4+** (5.6 recommended — the asset packs shortlisted below are current there).
2. New project → **Games → Third Person → C++** (not Blueprint-only; the sim is C++).
   Name it `DirtbagUE`, create it *inside this repo* so the `.uproject`, `Source/`,
   and `Content/` live alongside `Sim/` and `ROADMAP.md`.
3. Add Unreal's standard `.gitignore` for the project dirs (Binaries, Intermediate,
   DerivedDataCache, Saved) — same set landnam-ue uses.

## 2. Wire in the sim core

1. Add the files in `Sim/` to the game module (list them in the module's build
   rules or place the module's include path over `Sim/`). They are plain C++ —
   no engine includes — and must stay that way: the same translation units are
   compiled by `Sim/run-tests.sh`, which is what keeps the sim testable without
   the editor. Engine-facing wrappers (USTRUCT mirrors, Blueprint function
   libraries) go in `Source/`, never in `Sim/`.
2. Smoke test: from any actor, `dirtbag::Rng::FromSeed("grim-fjord-123")` and
   log five `NextDouble()` values — they must match the golden vector in
   `Sim/tests/test_main.cpp` exactly. If they don't, stop and find out why
   before building anything on top.

## 3. Phase 0 shopping list (Fab)

Two purchases, both cheap, both replaceable later:

1. **A basic climbing animation set.** Candidates from research (verify current
   price/reviews on Fab): *Climb and Vaulting Component V2* (20+ climb/vault
   animations, foot IK) — likely more than needed, which is fine; or any
   ledge-climb animation pack. Requirement: hang-idle, climb-up, reach
   left/right, a fall. Polish is Phase 2+'s problem; Phase 0 is blockout.
2. **Nothing else.** The gym room is BSP/blockout geometry. Resist the
   warehouse pack until Phase 1 — Phase 0's question is tension and
   legibility, and art hides the answer.

## 4. Phase 0 build order (suggested)

1. Blockout room + a 15° wall plane with 8–10 hold markers on a spline.
2. Character walks up (Third Person template as-is), interaction prompt,
   camera moves to the session frame.
3. Climb loop: advance hold-to-hold on the spline, playing the animation set;
   each move's outcome comes from `dirtbag::ResolveAttempt` — feed the whole
   attempt at session start, stage the returned timeline (odds → hesitation,
   pumpAfter → shake-outs and slowing, failure index → the fall).
   For anything beyond a single burn, go through `AttemptInSession`
   (`Sim/DirtbagSessionLoop.h`): it derives the attempt rng, applies warmup /
   remaining skin / psyche / project beta, and keeps the highpoint ledger —
   the session screen's "attempt 3, highpoint move 5" state comes from there,
   not from presentation-side bookkeeping.
4. Then make it *interactive*: run moves one at a time, HOLD TO CLIMB timing
   filling the per-move `execution` scalar. This is the moment Phase 0 exists
   for — the difference between watching a replay and driving an attempt.
5. Pump bar UI. Then the Done-when playtest (ROADMAP.md Phase 0).

## 5. House rules that carry over (from landnam-ue / CLAUDE.md discipline)

- `FMath::Rand` / `Math.random` equivalents stay banned in sim code — named
  streams from `DirtbagRng` only.
- Content additions must never touch engine code: routes/gear/jobs land in
  DataTables mirrored to `Sim/` structs.
- Any change to the sim that moves the golden vectors is save-breaking by
  definition and needs a deliberate decision, not a drive-by.
- `Sim/run-tests.sh` green before every commit that touches `Sim/`.

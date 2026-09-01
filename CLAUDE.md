# CLAUDE.md — Dirtbag (Unreal reimagining)

Climbing life-sim: live out of your van, work shifts, push your grade from gym plastic to the crag. The 2D game (dirtbag.rcjlabs.com, v0.956) is the proven reference implementation; this repo rebuilds it in Unreal Engine. Solo dev (Evan, RCJ Labs).

**The core design call — "2D minigames, 3D staging":** the character climbs; the player drives the attempt with the 2D game's real-time verbs (HOLD TO CLIMB, hold-to-load, pump bar); the sim arbitrates everything. No physical climbing simulation, no hand-IK. The watched session, the way LVDVS watches its fights.

## Start every session

1. Read `ROADMAP.md`. Find the `CURRENT MILESTONE` marker.
2. Work ONLY on that milestone unless told otherwise. Propose a plan before multi-file changes.
3. When the milestone's "Done when" criteria pass: update its status in ROADMAP.md, add a changelog line, advance the CURRENT MILESTONE marker, and commit.

Decision history lives in `concepts/` (brainstorm → finalists → the pivot). Read it when a design question feels reopened — it has usually been answered and dated there.

## Commands

- `tools/patch.py` — edit engine files through `edit(path, old, new)`, which raises unless the anchor matches exactly once. A scripted edit that silently matches nothing has twice removed or omitted a declaration while leaving its use behind, and with no Unreal compiler here that costs a whole build cycle to discover.
- `tools/preflight.sh` — **run before any commit touching `Sim/` or `DirtbagUE/Source/`.** Runs everything checkable without Unreal: the sim harness, a unity-build compile, the definition/bridge check, and the field-name check. Every one of those exists because a specific mistake reached Evan's PC and cost a build cycle; none of them is theoretical.
- `Sim/run-tests.sh` — build + run the sim core's standalone g++ harness, then compile all of `Sim/` as one translation unit the way UBT will. Must be green before any commit that touches `Sim/`.
- There is no Unreal *compiler* either, so engine-side C++ is written blind and Evan's PC is the first thing to compile it. That makes `tools/preflight.sh` the difference between a mistake costing seconds and costing a round trip — never push engine changes without it.
- There is no Unreal Editor in cloud/container sessions. Editor-side work (Blueprints, levels, animation wiring) is specified as precise checklists in `SETUP.md` or milestone notes for Evan to run locally — never guessed at, never marked done from here.

## Architecture (load-bearing rules)

**Engine-free sim core.** All game logic lives in `Sim/` as plain C++ — no Unreal types, no engine includes, ever. The same translation units compile in the UE module and in the g++ harness; that harness is what keeps the sim testable without the editor. Engine-facing wrappers (USTRUCTs, Blueprint libraries) live in `Source/`, never in `Sim/`.

**Sim/presentation split.** The sim decides; the renderer stages. `ResolveAttempt` returns a full per-move timeline (odds, pump, outcome) and the presentation layer acts it out. If it can be unit-tested, it doesn't belong in the presentation layer.

**Deterministic RNG.** Engine randomness (`FMath::Rand`, etc.) is banned in sim code. All randomness through `Sim/DirtbagRng` named streams (worldgen/session/events). The golden vectors in `Sim/tests/test_main.cpp` are frozen: any change that moves them is save-breaking by definition and needs a deliberate, logged decision.

**Data-driven content.** Routes, gear, jobs, events are data (DataTables mirrored by plain structs) — adding content must never touch engine code.

**The 2D game is the spec.** When a system's design is unclear, the answer is "what does the 2D game do?" — its balance is three years of tested truth. Deviations are pivots and get logged in `concepts/`, not improvised.

## Hard constraints

1. **Dials over magic numbers.** Tunable values live in dial structs (`SessionDials`) with a comment stating what the number means and why it's set there — the Sandbagged engine's dial-log style.
2. **Save discipline** (from the first save file onward): versioned save struct + migration registry; old saves must always load.
3. **Tests move with capabilities.** When the sim gains a mechanic, the harness gains its test in the same commit.
4. **Dirtbag voice everywhere.** Game text is dry, wry, second-person, climber-authentic, never jokey. The 2D game's changelog and route descriptions are the register to match.

## Style

- C++ per landnam-ue conventions (it's the sibling project and the method's proof).
- Small files, split by domain. Comments explain *why*, not *what*.
- Commit messages describe the change and its reason; ROADMAP.md changelog gets a line per milestone-relevant commit.

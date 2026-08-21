# SETUP — creating the Unreal project around the sim core

The container this repo is developed in has no Unreal Editor, so the sim core
(`Sim/`) is built and tested standalone (`Sim/run-tests.sh`, plain g++). These
are the steps only you can do, on your machine. The landnam-ue project is the
template for most of this — same method, second verse.

## 1. Create the project

1. Unreal Engine **5.4+** (verified plan below written against 5.8; the
   version only matters when buying asset packs — check each pack lists your
   engine version on Fab before purchase).
2. Pull this repo to your machine and check out the working branch first —
   the project must be created *inside the repo*:
   `git clone git@github.com:RCJLabs/Unreal-Game-2.git && cd Unreal-Game-2 && git checkout claude/dirtbag-unreal-port-ggvybe`
3. New project → **Games → Third Person → C++** (not Blueprint-only; the sim
   is C++). Name it `DirtbagUE`, and set the project *location* to the repo
   root — the launcher creates `Unreal-Game-2/DirtbagUE/` with the
   `.uproject`, `Source/`, and `Content/` inside it, alongside `Sim/` and
   `ROADMAP.md`.
4. Add Unreal's standard `.gitignore` under `DirtbagUE/` (Binaries,
   Intermediate, DerivedDataCache, Saved, `.vs`, `*.sln`) — same set
   landnam-ue uses. Commit and push the scaffold once it compiles clean, so
   container sessions can see the real `Source/` tree.

## 2. Wire in the sim core

> **Status (2026-08-15): steps 1–2 are committed to the repo** — the
> Build.cs include path, the four bridge files, and the smoke-test code in
> `DirtbagUEGameMode` all exist. Your part: pull, right-click
> `DirtbagUE.uproject` → *Generate Visual Studio project files*, build,
> press Play, and verify the log output in step 3.

The sim files are plain C++ with no engine includes, and must stay that way:
the same translation units are compiled by `Sim/run-tests.sh`, which is what
keeps the sim testable without the editor. Engine-facing wrappers (USTRUCT
mirrors, Blueprint function libraries) go in `Source/`, never in `Sim/`.

1. In `DirtbagUE/Source/DirtbagUE/DirtbagUE.Build.cs`, add `using System.IO;`
   at the top and this line in the constructor so `#include "DirtbagRng.h"`
   resolves everywhere in the module:
   `PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../../Sim"));`
2. UBT only compiles `.cpp` files that live under the module, so add one
   thin bridge file per sim translation unit in
   `DirtbagUE/Source/DirtbagUE/` — this keeps the TUs identical to the
   harness rather than merging them:
   - `SimRng.cpp` → `#include "../../../Sim/DirtbagRng.cpp"`
   - `SimCore.cpp` → `#include "../../../Sim/DirtbagCore.cpp"`
   - `SimSession.cpp` → `#include "../../../Sim/DirtbagSession.cpp"`
   - `SimSessionLoop.cpp` → `#include "../../../Sim/DirtbagSessionLoop.cpp"`
3. Smoke test — prove the port is bit-exact before building anything on top.
   In the template's GameMode, override `BeginPlay` and log the golden seed:

   ```cpp
   #include "DirtbagRng.h"

   void ADirtbagUEGameMode::BeginPlay() {
     Super::BeginPlay();
     dirtbag::Rng rng = dirtbag::Rng::FromSeed("golden");
     for (int i = 0; i < 5; i++) {
       UE_LOG(LogTemp, Display, TEXT("golden[%d] = %.17g"), i, rng.NextDouble());
     }
   }
   ```

   Press Play and check the Output Log for exactly:
   ```
   golden[0] = 0.59432327281683683
   golden[1] = 0.94342079781927168
   golden[2] = 0.30383505066856742
   golden[3] = 0.76765315467491746
   golden[4] = 0.47240672400221229
   ```
   (The same five values are frozen in `Sim/tests/test_main.cpp`.) If they
   don't match, stop and find out why before building anything on top.

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

## 3a. Vendored Fab packs (not in the repo)

Some purchased packs are too big to version and are ignored in `.gitignore`.
A fresh clone will not have them, and any level or Blueprint referencing them
will show missing references until they are reinstalled from the Fab library
in the Epic launcher, into `DirtbagUE/Content/` under the pack's own name.

| pack | folder | size |
| --- | --- | --- |
| PWL Light Manager | `Content/PWL_Light_Manager/` | 235 MB |
| Procedural Building Generator | `Content/ProceduralBuildingGenerator/` | 681 MB |

**Why they are ignored rather than tracked in LFS.** `.gitattributes` routes
`*.uasset` and `*.umap` through Git LFS, which is right for our own content —
Blueprints, levels, the climb-wall assets — because those are small and change
often. It is wrong for these: 916 MB is the entire free LFS tier (~1 GB
storage, ~1 GB/month bandwidth) spent in a single commit, one fresh clone
would exhaust the month's bandwidth, and **LFS storage is not reclaimed by
deleting the files or rewriting history** — the objects stay and the quota
stays spent. Against that, reinstalling from the launcher costs five minutes.

Add a row here whenever a pack is added to the ignore list, so the reinstall
list stays complete rather than living in somebody's memory.

## 4. Phase 0 build order (suggested)

> **The sim is already exposed to Blueprint** (`Source/DirtbagUE/
> DirtbagSimTypes.*`, `DirtbagSimLibrary.*`): search the node menu for
> "Dirtbag". Do not re-implement any sim behavior in Blueprint — if a node
> is missing, ask for it and it gets added to the wrapper (and, if needed,
> the sim + its tests) container-side.

0. Import the purchased climbing animation set (Hammerhead). If it targets
   the UE5 skeleton it drops straight onto the template character; otherwise
   use the auto-retargeter (right-click the anims → Retarget Animations).
   Needed for Phase 0: hang idle, a climb-up/reach for each side, a fall.
1. Blockout room + a 15° wall plane with 8–10 hold markers on a spline.
2. Character walks up (Third Person template as-is), interaction prompt,
   camera moves to the session frame.
3. Staged replay first: a `BP_ClimbSession` actor calls **Build Route**
   (worldgen seed + route name/grade/type) → **Start Session** (climber) →
   **Attempt In Session** (bot-driven). Stage the returned `Timeline` on the
   spline: `Odds` → hesitation before the move, `PumpAfter` → shake-outs and
   slowing, first failed entry → the fall onto the mat; `bSent` → top-out.
4. Then make it *interactive* — the moment Phase 0 exists for: swap step 3's
   resolve for **Begin Live Attempt**, then per move **Peek Odds** (drive
   the tension readout while HOLD TO CLIMB charges), **Step Move** with the
   timing-derived execution scalar (0..1) on commit, **Shake Out** on
   release at a stance (first shake is the stance's value; milking it
   diminishes and pays a hang tax). When **Is Over**: **Finish** for the
   result card, then **Commit To Session** so skin/warmth/psyche and the
   project ledger update — never bookkeep those presentation-side.
5. Pump bar UI bound to the live attempt's **Get Pump**. Then the Done-when
   playtest (ROADMAP.md Phase 0).
5. Pump bar UI. Then the Done-when playtest (ROADMAP.md Phase 0).

## 5. House rules that carry over (from landnam-ue / CLAUDE.md discipline)

- `FMath::Rand` / `Math.random` equivalents stay banned in sim code — named
  streams from `DirtbagRng` only.
- Content additions must never touch engine code: routes/gear/jobs land in
  DataTables mirrored to `Sim/` structs.
- Any change to the sim that moves the golden vectors is save-breaking by
  definition and needs a deliberate decision, not a drive-by.
- `Sim/run-tests.sh` green before every commit that touches `Sim/`.

# ROADMAP — Dirtbag (Unreal reimagining)

Working title **Dirtbag** (3D). Decision log and concept detail live in `concepts/`.
Convention (house style): numbered phases, explicit **Done when** gates, one CURRENT MILESTONE marker. Work only on the current milestone; when its criteria pass, update status, add a changelog line, advance the marker, commit.

**The design in one line:** the 2D game's life-sim and session mechanics ("2D minigames, 3D staging" — the character climbs, the player drives the attempt with HOLD TO CLIMB / hold-to-load / pump-bar verbs, the sim arbitrates), staged in a 3D valley with a gym, crags, the Lot, the van, and the dog.

**Architecture rules (carried from the house method):**
- Sim core is engine-free C++ in `Sim/` — plain structs and pure functions, no Unreal types. The identical translation units compile in the UE module and in the standalone g++ harness (`Sim/run-tests.sh`). The 2D game is the reference implementation and balance lab.
- `Math.random` equivalents are banned; all randomness through the seeded named-stream RNG in `Sim/DirtbagRng.*`.
- Content is data, never code: routes, gear, jobs, events land in DataTables mirrored by plain structs.
- Save shape changes bump a SAVE_VERSION and ship a migration, from the first save onward.

---

## Phase 0 — Session Proof   ✅ DONE 2026-08-17

The one question this phase answered: **is a minigame-driven session tense and legible on a 3D wall?** — **Yes.** Gates passed on the blockout wall: the stat-block test (same route, two climbers, visibly different attempts, zero animation changes), the tension test (a pumped HOLD TO CLIMB attempt at the limit feels tense), and the watcher test (a V4 and a V7 distinguishable by watching alone). The kill-switch question is retired; from here the risk profile is ordinary game production.

Scope: a blockout gym room (no art pass), third-person walk-up, tap the wall to enter a session. The climber ascends a route spline hold-to-hold using a basic climbing animation set. The pump bar and the HOLD TO CLIMB verb (hold to move, release to shake out) drive per-move resolution through the C++ session sim. Falls and sends both staged: the peel-off, the mat landing, the top-out.

Container-side groundwork (done first, no editor needed): `Sim/` core — RNG, core types, session resolver — with golden-vector tests green in the standalone harness. Editor-side work follows `SETUP.md`.

**Done when:**
1. A watcher can tell how close an attempt was without reading a number (playtest on someone who isn't the dev).
2. A deliberately pumped attempt feels tense through the minigame alone — the 2D verbs survive the transplant.
3. The same route resolves visibly differently for two climber stat blocks with zero animation changes (the sim, not the staging, decides).
4. `Sim/run-tests.sh` green; session resolution is deterministic per seed.

## Phase 1 — Gym Slice   **<<< CURRENT MILESTONE**

The 2D game's gym day-loop in 3D: wake in the van (static scene), drive to the gym, session with multiple routes and grades, rest/eat, work a shift, sleep, bills land. Skills creep, pump/skin budgets across a week. First save file (versioned).
**Done when:** a 7-day loop plays start to finish; a session on a too-hard route and a too-easy route both *read* correctly; save/load round-trips.

## Phase 2 — First Crag & the Lot

Roadside Crag outdoors (Megascans rock, prime-conditions window staged as moving light/shade), the Lot with campfire and two named neighbors, the dog, ~25 named routes + 3 open project lines (clean → work → send → name), 3 partners with their own careers.
**Done when:** a full season plays; a new player names their own first ascent inside 6 hours; the window mechanic demonstrably changes when players choose to burn attempts.

## Phase 3 — Town & Career

Town block (gear shop, diner), gear economy with lean/wear, odd-jobs board, the salaried-job trap, factions v1, van wear + breakdowns on crag drives.
**Done when:** the money loop pressures the climbing loop the way the 2D game's does (measured against the 2D reference balance).

## Phase 4 — The Long Game

Sport/rope presentation, crags 2–3, ethics arcs, sponsorship, injuries/aging, legacy. Early Access decision at phase start.

---

## Changelog

- 2026-08-17 — Reading the line from the ground (`ReadRoute` → Warmup / Comfortable / At your limit / Project / Not this year, judged against the *guidebook* grade so a sandbag reads honest until you're on it) and fatigue that actually arrives: energy now scales with how far the line is above you, and its penalty fades in from `freshEnergy` instead of waiting at a threshold no session ever reached. A limit session now has an arc — burn twelve is worse than burn two, on thin skin and low nerve. Measured before building: a day clock was considered and dropped, since skin ends a session at burn 8 while the clock reads 9am.
- 2026-08-17 — **Balance: the skill ladder recalibrated** (`concepts/BALANCE-SKILL-LADDER.md`). Simulating a month exposed that `skill/100*18` made a default 50-stat climber a V9 — nothing on a V0–V7 board resisted anyone (45-stat climbers sent V7 74% of the time). New `SkillToGrade` dial (span 14, floor −2) makes skill 50 a V5 climber, shared by resolver and trainer instead of duplicated. Training retuned to match (challenge gate now functional, diminishing returns, endurance no longer 50× faster than everything else). Two tests recalibrated, `TestGradesResist` added so the ladder can't silently re-flatten. Money loop deliberately untouched — that's Phase 3's gate against the 2D reference.
- 2026-08-17 — Central game state: `UDirtbagGameInstance` (registered in DefaultEngine.ini) owns seed/player/day, loads on boot, saves on sleep, serves the cached gym board and per-day session seeds. Climb wall retrofitted: with the game instance present it carries board problem #BoardIndex and commits attempts day-integrated (standalone fallback kept for test maps); session HUD gains a DAY/$/ENERGY row. New `ADirtbagSleepSpot`: walk up, E, day ends, save writes.
- 2026-08-17 — Day loop + save reach Blueprint: `FDirtbagPlayerState`/`FDirtbagDayState` mirrors, day-verb nodes (Wake Up, Pass Hours, Eat Meal, Work Shift, Start Gym Session, Sleep To Next Day, Gym Board), day-integrated attempt paths (Day Attempt for replay, Begin Day Live Attempt / Commit Live Attempt for HOLD TO CLIMB), and Save/Load To File under Saved/SaveGames — serialization stays in Sim/, nodes only move bytes.
- 2026-08-17 — Phase 1 groundwork: day-loop sim core (`Sim/DirtbagDay.*` — day clock, hunger/energy/cash, meals and shifts, weekly bills, overnight skin regen and psyche drift, training creep from hard attempts, deterministic gym route board) and the first save file (`Sim/DirtbagSave.*` — versioned key=value serialization of career state, exact double round-trip, future-version refusal, migration registry live and tested from v1). Harness grows twelve matching tests including a 7-day integration loop.
- 2026-08-17 — **Phase 0 — Session Proof: DONE.** All four gates passed on the blockout wall (stat-block, tension, watcher, deterministic sim). The session-model pivot is validated. CURRENT MILESTONE advances to Phase 1 — Gym Slice.
- 2026-08-17 — HOLD TO CLIMB goes interactive: ADirtbagClimbWall gains a live mode driven by the live-attempt API — hold Space to charge the grip meter, release in the sweet window to latch (execution from timing), release early to shake out, over-grip and the move fires itself badly. Debug HUD (pump / grip / best-case odds), mount + top-out anims, sim-side commit of live attempts. Verb dials on the actor; replay mode kept as a toggle.
- 2026-08-15 — Engine wrapper layer (`Source/DirtbagUE/DirtbagSimTypes.*`, `DirtbagSimLibrary.*`): USTRUCT/UENUM mirrors and Blueprint nodes over route gen, the session loop, and the live attempt. Sim-side, `AttemptInSession` decomposed into `DeriveAttemptRng`/`BuildSessionAttemptInput`/`CommitAttempt` so live attempts commit through the same accounting (equivalence pinned by test). SETUP.md §4 rewritten as a Blueprint-level checklist.
- 2026-08-15 — Live attempt API (`BeginAttempt`/`PeekOdds`/`StepMove`/`ShakeOut`/`FinishAttempt`): the resolver unrolled so HOLD TO CLIMB can drive move-by-move, with release-to-shake as a sim-arbitrated verb (diminishing returns + hang tax). `ResolveAttempt` reimplemented as a bot on the live core — equivalence pinned by test. Deliberate delta: `peakPump` now records the true pre-recovery peak.
- 2026-08-15 — Session loop layered over the resolver (`Sim/DirtbagSessionLoop.*`): warmup → attempts → skin budget within a day, plus `ProjectMemory` across days (highpoint ledger, beta learned from touched moves, first-send style). Resolver gains cold-start and psyche terms, neutral by default — existing outcomes and golden vectors untouched. Harness grows six matching tests.
- 2026-08-15 — Project decided: Dirtbag reimagining, session-model pivot ("2D minigames, 3D staging"). Repo scaffolded: concepts, roadmap, `Sim/` core (RNG + types + session resolver) with standalone test harness, `SETUP.md`.

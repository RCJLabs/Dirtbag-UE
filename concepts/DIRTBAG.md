# DIRTBAG — Concept Doc (Unreal reimagining)

*Finalist deep-dive, 2026-08-15. Companion: [THREE-WINTERS.md](THREE-WINTERS.md), [COMPARISON.md](COMPARISON.md).*

> A climbing life-sim. Live out of your van, work shifts, and push your grade from gym plastic to the crag — now in first/third person, where the wall is real rock and the shade line actually moves.

---

## 1. Vision & pillars

The 2D game's design is proven at v0.956 and ships on Google Play. The reimagining keeps its soul and changes one thing: **you climb with your hands instead of rolling dice.** Everything else is a port of systems that already work.

**Pillars, in priority order:**

1. **The life, not the score.** Grades are the visible progress bar; the subject is what chasing them costs — body, money, relationships, integrity. The van does not care which kind of day it was.
2. **Your body is the save file.** Skin, pump, training load, injury, age. Every resource is physical and most of them are visible on the character.
3. **Climbing is played, not passed.** No send-odds roll. The sim decides what your body *can* do (grip strength remaining, reach, friction); the player decides what it *does* do.
4. **Conditions are content.** The daily prime window — temperature, humidity, sun angle — is the day's structure. Waiting in the shade for the 4pm window is gameplay.
5. **Ethics are gameplay.** Style (onsight > flash > redpoint), chipping, bolting, closures, sponsorship compromise — multi-beat arcs whose consequences resurface seasons later.
6. **The scene remembers.** Named partners with their own careers, factions with opposed values, a crew the town names without asking you, a dog.

## 2. Player fantasy & core loops

**Minute-to-minute (the climbing loop).** Read the line from the ground. Rack up / pad down. Climb: reach, weight, and grip decisions on real holds, pump building in the forearms, shake-outs at rests, the choice to settle into a hold or move now. Fall onto pad or rope, lower, brush, try again — or walk away and save skin for the window.

**Day loop (ported from 2D).** Wake in the van. Check weather and the prime window. Spend the day's hours: work a shift, run an odd job, train, drive to a crag, climb, socialize at the Lot fire. Sleep; bills land.

**Season/career loop (ported from 2D).** Skills and grade creep up; money mostly doesn't. Projects fall or become nemeses. Factions pull; sponsors call; the salaried-job trap tempts. Injuries and age bend the curve. Retire eventually — legacy tallied, next generation inherits.

## 3. The central design call: 2D minigames, 3D staging

> **PIVOT (2026-08-15, decided):** climbing is **not** physically player-driven. An earlier draft of this doc proposed full physical climbing gated on hand-IK feel; Evan pivoted to the session model below, which keeps the 2D game's proven mechanics and removes the animation-fidelity risk entirely.

Sessions work the way the 2D game already works — and the way LVDVS's fights work: **the character climbs; the player drives the attempt.** The climber moves hold-to-hold along an authored route via a basic climbing animation set. The 2D game's real-time verbs are the interaction layer: **HOLD TO CLIMB** (release to shake out), **hold-to-load** (release too early and you come up short, load past it and you barrel off), the pump bar, STICK IT / THROW / LOCK beats at cruxes. Minigame performance feeds the sim as a per-move execution quality; the sim — the same odds/body model the 2D game balanced over three years — remains the arbiter of what happens.

| 2D input | 3D expression (session model) |
|---|---|
| Skills vs grade | Per-move difficulty vs. stat block — unchanged from 2D, resolved per move instead of per attempt |
| Pump | The pump bar, now with the climber visibly slowing, over-gripping, chalking frantically |
| Skin | Per-session budget; tape appears on the character's hands |
| Prime window | Modifier as in 2D — but *staged*: light and shade visibly cross the wall through the day |
| Morphology | Per-move fit modifiers (the scrunchy crux vs. the long reach) — expressed in text and outcome, not custom animation |
| Psyche / head | Fall-commitment beats above gear; camera and breath do the acting |
| Gear lean/wear | Same modifiers as 2D |
| Route type | Hold props on the wall set dressing the move types (crimp/sloper/pinch/pocket/dyno/crack) |

What 3D adds is **staging**, not simulation: watching your climber inch up a real wall, the fall that everyone in the gym turns to watch, tick marks visible on the holds, the shade line you're waiting for. **Sandbagging survives** (public grade vs hidden trueGrade). **Ascent style survives** as the score: onsight > flash > redpoint > sent, and cheapening actions stay visible on the wall.

## 4. Systems map

**Port wholesale (design already proven, engine-agnostic):**
factions (4, opposed), persistent partners with their own careers, the Lot neighbors and campfire, van as wearing machine (6 components, patch-vs-replace), gear brands/lean/wear, odd-jobs board, salaried-job trap + Dirtbag Year achievement, dreams (Rig/War Chest/Home Base), projecting/nemesis tracking, injury + physio + aging, legacy/generations, the dog, the crew-naming system, ethics arcs.

**Redesigned for 3D:** the climbing loop itself (above); skin/pump embodiment; morphology; weather/light as world state, not a modifier line.

**Cut or defer past 1.0:** comps/Olympics, expeditions (El Cap tier — this is THE WALL's design, fold in later), deep-water solo, big-wall multi-day, filmmaking/photography economy, minigames (poker etc. — keep ONE campfire game), gym ownership, Solo mode, Notown/Halloween. The 2D game took years to accrete these; the 3D game earns them the same way.

## 5. The session proof (replaces the old FBIK pathfinder)

The pivot retires the "does physical climbing feel good?" question. The remaining question is smaller and safer: **"is a minigame-driven session tense and legible on a 3D wall?"** That is Phase 0 of [ROADMAP.md](../ROADMAP.md) — a blockout gym room, one route spline, a basic climbing animation set, the pump bar and one HOLD TO CLIMB beat wired to the C++ session sim. Its Done-when criteria live in the roadmap; the short version is that a watcher should feel how close the attempt was without reading a number.

THE HOLLOW remains available as a later shippable spin-off, but it is no longer needed as a tech pathfinder — the tech this game needs is now mostly purchasable, and the sim core is buildable (and testable) before the Unreal project even exists.

## 6. MVP scope & cut ladder

Ship order, each rung a coherent game:

1. **One crag + the Lot + the van + the dog.** Bouldering only (no rope systems). Shifts, odd jobs, skin/pump/window loop, 3 named partners, ~25 routes + 3 open project lines.
2. **+ Town block** (gym, gear shop, diner), sport climbing (rope/bolts), factions live.
3. **+ Second and third crags** (rock-type variety), trad, van travel with wear/breakdowns.
4. **+ Ethics arcs, sponsorship, legacy/generations.**

## 7. Fab shopping list (verified 2026-08)

**Exists — buy, don't build:**
- ~~**Climbing systems (as scaffolding):**~~ **OBSOLETE 2026-08-22 — do not
  buy.** CLAUDE.md's core design call retired this whole category: *"no
  physical climbing simulation, no hand-IK."* The climber interpolates along
  a spline playing back six authored loops, and `ClimbingAnimationSet`
  (already owned) covers them. The largest saving on this list, and it comes
  from a decision rather than a purchase. Kept below for the record: [Procedural Climbing with Control Rig](https://www.fab.com/listings/9a460f95-7079-48b6-b38f-e17d764d4f34) — Full Body IK placement with *no canned animations*, the closest match to our model; [Climb and Vaulting Component V2](https://www.fab.com/listings/a32f69ab-aefd-4d38-869f-b5414364aca1) and Dynamic Ledge Climb System as animation-driven references; [Dragon IK Plugin](https://www.fab.com/listings/d3f8d256-d8d9-4d27-91c1-c61e55e984a6) as a general IK fallback.
- **Rock/terrain:** Quixel Megascans. ⚠️ **"free for UE" is stale as of
  2026-08-22** — free Megascans ended 31 December 2024, the library moved to
  Fab, and assets are priced individually (a subset stays free; Megaplants
  are free). Still the right choice for the rock and still worth buying, but
  it is a line item now rather than a freebie. Cliffs, boulders, canyon
  surfaces solved.
- **Locomotion base:** Epic's free Game Animation Sample (motion matching, 500+ ground animations) for everything that isn't climbing.
- **The dog:** [DOG on Fab](https://www.fab.com/listings/5a41c4ec-d50f-45e0-8384-c6dc905468c5) — 26 animations incl. sniff, howl, rest, sleep. The emotional register is purchasable.
- **Town/vehicles:** small-town packs exist (Americana is a thinner category than European rural — budget shopping time); [City Sample Vehicles](https://www.fab.com/listings/2909157b-ddfa-4cef-a925-69dc2467021f) free for drivable bases; camper van models exist individually.

**Custom work (the real budget, post-pivot):**
- The session sim in engine-free C++ (per-move resolution, pump/skin/window/morphology) — home-turf work, testable headless.
- Session staging: camera, minigame UI, animation state machine on route splines. *This is now the project's craft center.*
- Hold library (sculpt ~40 hold archetypes as set dressing; far lower fidelity bar than physical climbing needed).
- Rope/belay presentation (defer to MVP rung 2).
- Character customization (2D game has 10 bodies × 29 hairs etc.; start far smaller).

## 8. Tech notes (consistent with the landnam-ue method)

- **Sim core in plain C++, free of Unreal** — needs/economy/career/injury/aging as pure functions over a serializable state, compilable in a standalone harness against golden vectors, exactly like landnam-ue's parity setup. The 2D game becomes the *reference implementation* for every ported system.
- **Content in DataTables/DataAssets:** routes, holds, gear, jobs, events, dialogue — adding content never touches engine code (the house rule, kept).
- **Deterministic named RNG streams** (worldgen/events/social) for the sim layer; climbing itself is deterministic physics + player input, no rolls.
- **Climbing stack:** Control Rig + FBIK, hold actors with typed sockets, a grip-strength model in the C++ sim consumed by the animation layer.
- **Save discipline:** versioned save struct + migration registry from day one (the 2D game is at SAVE_VERSION 31; the habit transfers).

## 9. Risks & mitigations

| Risk | Odds | Mitigation |
|---|---|---|
| Session isn't tense/legible in 3D (the watched climb reads as a cutscene) | The new top risk | Phase 0 gate with written criteria; the 2D verbs are proven, only the staging is new |
| Basic climbing animations look janky on varied walls | Moderate, tolerable | Authored route splines + curated animation pack; stylized character art lowers the bar |
| Scope creep from the 2D game's 40k+ lines of content | Certain if unmanaged | The cut ladder is the contract; 2D game stays live as the "full" experience |
| Americana asset gap | Minor | Town is small and stylized; kitbash European-rural + custom signage |
| Solo-dev burnout on an L project | Real | Stage B ships a whole game inside year one |

## 10. Rough milestones

- **M0 (weeks 1–8):** Stage A prototype → go/no-go.
- **M1 (months 3–8):** The Hollow, shipped.
- **M2 (months 8–14):** Dirtbag MVP rung 1 (one crag + Lot + van + dog), playable end-to-end.
- **M3+:** rungs 2–4, Early Access candidate at rung 2.

## 11. Open questions for Evan

1. First person, third person, or both? (Climbing reads best first-person; the town and the dog read best third.)
2. Art direction: stylized (cheaper, hides IK sins, ages well) vs realistic (Megascans pulls this way)? Recommend **stylized characters on realistic rock** — it's also a distinctive look.
3. Does the 2D game's town/canon carry over (same crag names, same NPCs) or is this a fresh valley?
4. PC-first (Steam) with gamepad, or still chasing mobile? (Recommend PC-first; the 2D game already owns mobile.)

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

## 3. The central redesign: from odds to body

The 2D game assembles a send probability from ~a dozen modifiers. The 3D game keeps **every one of those inputs** but re-routes them from *dice* to *simulation constraints*:

| 2D input | 3D expression |
|---|---|
| Skills vs grade | Max hang time per hold type, lock-off strength, reach envelope |
| Pump | A real forearm meter that recovers at rests and drains per move; redline = fingers open |
| Skin | Per-session budget; thin skin reduces friction on crimps, visible tape |
| Prime window | Global friction coefficient driven by temp/humidity/sun; visibly better sticking |
| Morphology | Literal reach, literal body positions — Compact fits the scrunchy crux Lanky can't |
| Psyche / head | Camera behavior, breathing audio, and fall-commitment above gear |
| Gear lean/wear | Shoe edging/smearing modifiers, pad size, rope drag |
| Route type | Actual hold shapes: crimp, sloper, pinch, pocket, dyno, crack |

**Sandbagging survives** (public grade vs hidden true grade — the wall doesn't lie, the guidebook does). **Ascent style survives** as the score: tick marks and brushed holds visibly cheapen a line, and yours are visible to others.

## 4. Systems map

**Port wholesale (design already proven, engine-agnostic):**
factions (4, opposed), persistent partners with their own careers, the Lot neighbors and campfire, van as wearing machine (6 components, patch-vs-replace), gear brands/lean/wear, odd-jobs board, salaried-job trap + Dirtbag Year achievement, dreams (Rig/War Chest/Home Base), projecting/nemesis tracking, injury + physio + aging, legacy/generations, the dog, the crew-naming system, ethics arcs.

**Redesigned for 3D:** the climbing loop itself (above); skin/pump embodiment; morphology; weather/light as world state, not a modifier line.

**Cut or defer past 1.0:** comps/Olympics, expeditions (El Cap tier — this is THE WALL's design, fold in later), deep-water solo, big-wall multi-day, filmmaking/photography economy, minigames (poker etc. — keep ONE campfire game), gym ownership, Solo mode, Notown/Halloween. The 2D game took years to accrete these; the 3D game earns them the same way.

## 5. The pathfinder plan (the de-risking centerpiece)

The project's only existential question is **"does climbing feel good?"** Answer it before betting anything on it.

**Stage A — gray-box prototype (4–8 weeks).** One 15m gray-box wall with typed holds. Character with hand/foot placement via Full Body IK, pump meter, shake-outs, falls. No town, no art, no save system.
**Go/no-go criteria, written down before starting:**
- Moving between 10 holds feels deliberate, not floaty, within 5 seconds of picking up the controller.
- A pumped-out fall reads as *earned* (player saw it coming ≥2 moves out).
- One route can be meaningfully easier/harder by hold selection alone, no stat changes.
- A morphology swap (reach ±10%) changes the beta on at least one sequence.
- Frame budget holds on target hardware with FBIK active.

**Stage B — if go: THE HOLLOW as the shippable pathfinder.** The 3–4 hour slot-canyon ghost story (BRAINSTORM #7) ships the tech in a real game: hold interaction, rope basics, canyon lighting, one location. Revenue and audience while Dirtbag proper is designed.
**Stage C — if no-go:** the prototype cost weeks, not the project. Fall back to THREE-WINTERS with full confidence and no sunk-cost drag.

## 6. MVP scope & cut ladder

Ship order, each rung a coherent game:

1. **One crag + the Lot + the van + the dog.** Bouldering only (no rope systems). Shifts, odd jobs, skin/pump/window loop, 3 named partners, ~25 routes + 3 open project lines.
2. **+ Town block** (gym, gear shop, diner), sport climbing (rope/bolts), factions live.
3. **+ Second and third crags** (rock-type variety), trad, van travel with wear/breakdowns.
4. **+ Ethics arcs, sponsorship, legacy/generations.**

## 7. Fab shopping list (verified 2026-08)

**Exists — buy, don't build:**
- **Climbing systems (as scaffolding):** [Procedural Climbing with Control Rig](https://www.fab.com/listings/9a460f95-7079-48b6-b38f-e17d764d4f34) — Full Body IK placement with *no canned animations*, the closest match to our model; [Climb and Vaulting Component V2](https://www.fab.com/listings/a32f69ab-aefd-4d38-869f-b5414364aca1) and Dynamic Ledge Climb System as animation-driven references; [Dragon IK Plugin](https://www.fab.com/listings/d3f8d256-d8d9-4d27-91c1-c61e55e984a6) as a general IK fallback.
- **Rock/terrain:** Quixel Megascans — free for UE. Cliffs, boulders, canyon surfaces solved.
- **Locomotion base:** Epic's free Game Animation Sample (motion matching, 500+ ground animations) for everything that isn't climbing.
- **The dog:** [DOG on Fab](https://www.fab.com/listings/5a41c4ec-d50f-45e0-8384-c6dc905468c5) — 26 animations incl. sniff, howl, rest, sleep. The emotional register is purchasable.
- **Town/vehicles:** small-town packs exist (Americana is a thinner category than European rural — budget shopping time); [City Sample Vehicles](https://www.fab.com/listings/2909157b-ddfa-4cef-a925-69dc2467021f) free for drivable bases; camper van models exist individually.

**Custom work (the real budget):**
- Hand-to-hold contact quality on top of FBIK (finger poses per hold type: crimp/sloper/pinch/pocket/jam). *This is the project.*
- Hold library (sculpt ~40 hold archetypes, scatter via Megascans surfaces).
- Rope/belay simulation (defer to MVP rung 2).
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
| Climbing feel never gets good | The big one | Stage A gate with written criteria; cheap to fail |
| Hand IK uncanny valley | High effort, known solutions | Procedural-climbing asset as base; stylized-not-photoreal character art lowers the bar |
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

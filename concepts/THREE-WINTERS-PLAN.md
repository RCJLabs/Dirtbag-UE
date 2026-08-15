# THREE WINTERS — Production Plan

*2026-08-15. Concept: [THREE-WINTERS.md](THREE-WINTERS.md) · Rival plan: [DIRTBAG-PLAN.md](DIRTBAG-PLAN.md) · Decision aid: [COMPARISON.md](COMPARISON.md)*

Format follows the Landnám ROADMAP convention: numbered phases, explicit **Done when** gates, a CURRENT MILESTONE marker. If Three Winters wins the decision, this file *is* the starting roadmap.

---

## Pros & cons (the honest list)

**Pros**

1. **The heart wants it, and that's not nothing.** A solo project runs on years of morning motivation. "I get to build saga Iceland today" is fuel; do not discount it as unserious.
2. **Best asset coverage of any idea on the board.** Real Iceland heightmaps from 2 m survey data, three modular Viking village packs *with interiors*, MetaHuman-ready armor, villager clothing packs, horse systems, snow-trail systems, combat frameworks. Week 3 of this project looks *gorgeous* for a few hundred dollars.
3. **Massive reuse from the flagship.** Landnám's feud/wergild logic, winter math, vague-forecast trick, event-card pipeline, and chronicle voice are Three Winters parts. The landnam-ue repo has already solved the C++ sim-core method, parity testing, and hex library. Every future Landnám improvement is free R&D for this game.
4. **The niche is genuinely empty.** "Viking game" is crowded (Valheim, AC Valhalla, God of War) — but every one of them is mythology. *Saga realism* — outlawry, wergild, the Þing, no monsters — has never been done. The sagas are literally sitting there, public domain, being nobody's game.
5. **Home-turf difficulty.** The hard parts (shelter/trust network, pursuit sim, social consequence) are *systems in C++* — exactly the kind of problem every prior RCJ project has solved and tested well.
6. **Structurally bounded.** Three in-game years, one district, a fixed cast. Unlike most open-world RPGs, the design has walls built in.

**Cons**

1. **Content volume is a slow-failing risk.** 18 households, 6 hunters, 120 events, 30 winter scenes is a *disciplined* budget — and still a mountain of authored content. If the templates don't carry enough weight, the grind reveals itself at month 12+, the most expensive possible moment. This is the classic solo-dev killer, and no phase gate fully de-fangs it.
2. **The Skyrim comparison cuts both ways.** "Viking Skyrim" sets expectations no solo game can meet (radiant quests, dungeon count, voice acting). Marketing has to frame it as a survival-saga — closer to a systemic Pentiment-meets-The-Long-Dark — and some players will bounce off the honesty.
3. **Melee has to be good enough.** Combat is rare by design, but rare means each fight carries weight. A mediocre holmgang undermines the whole lethality thesis. Buying a framework helps; feel-tuning is still real work.
4. **Three interacting new systems.** Shelter/trust + pursuit + stealth-lite must mesh. Systemic games fail in the seams; the vertical slice exists to find those seams at month 5, not month 14.
5. **No existing audience bridge.** Dirtbag's players are climbing/life-sim people; Landnám's web audience is small. This game starts its Steam presence from zero (canon-linking it to Landnám helps, a little).
6. **Delays the game only Evan can make.** Anyone (in theory) could make the saga RPG. Nobody else will make Dirtbag. Opportunity cost is a real con — the niche could stay empty for years, but it only takes one A24-style indie to claim it.

## Development phases

**>>> CURRENT MILESTONE: Phase 0 (not started — pending the Dirtbag-vs-Three-Winters decision) <<<**

### Phase 0 — Look & traversal proof (weeks 1–6)

Assemble the purchasable game: one Iceland heightmap valley, one village pack farm with interior, Ultra Dynamic Sky driving weather and a snowstorm, Game Animation Sample locomotion, a horse system, snow trails. Walk from the fell down to the farm in failing light, in the wind, and step into the hall.

Also in this phase: **the travel-model decision** — full free-roam across the district vs. hybrid (free-roam valleys + Landnám's hex layer for long crossings). Prototype both crossings crudely; pick one and log the DECISION.

**Done when:**
1. The walk-to-the-hall slice passes the screenshot test (a cold shot posted with no context earns a "what is this?").
2. Frame budget holds in the snowstorm with trails active.
3. Travel model decided and logged with reasoning.
4. A one-page "why this isn't Skyrim" positioning statement is written — the marketing frame gets set before the game does.

### Phase 1 — Sim spine (months 2–5)

The game's actual machinery, in plain C++, free of Unreal, headless-testable against golden vectors (the landnam-ue method, reused verbatim): calendar/seasons with the vague long-range forecast; the **shelter/trust network** (per-household disposition: fear/sympathy/greed, risk tolerance, memory of what you did and what they've heard); the **pursuit sim** (named hunters with routes, informants, information decay); wergild ledger and Þing-case state; the saga-log generator ported from Landnám's chronicle system.

**Done when** a headless run simulates three full years from a seed — player modeled by a simple policy bot — and produces a coherent printed saga; a household or hunter is added via DataTable rows only; and the pursuit sim demonstrably converges on the player's actual habits (camp twice in the same valley, the hunters learn it).

### Phase 2 — Vertical slice: one valley, one winter, one feud (months 5–9)

Playable end to end: trial/character creation (who you killed, why it was arguably fair) → one summer among ~6 farms under a false name → autumn den choice (farm outbuilding / cave / island) → one full winter in the den with authored scenes → spring Þing hearing that moves your case. One named hunter active throughout. Melee framework integrated and stripped (first holmgang authored). ~40 event cards live.

**Done when** a stranger can play trial → spring in one sitting, recount what happened *as a story* unprompted, and name the hunter. The seams between shelter, pursuit, and stealth systems are cataloged and either fixed or cut.

### Phase 3 — Full district (months 9–16)

18 households via the template system (each hand-touched: one authored secret apiece), the full hunter cast of 6, year 2 content (~40 more events), feud arcs 2–3, the district Þing with sympathizer/mitigation mechanics. **Early Access candidate at the end of this phase.**

**Done when** year 1 → 2 plays through with the full cast, the content budget spreadsheet shows actuals vs. plan (the tripwire: if events are landing over 2× their time estimate, cut scope *now*, at the cheap moment), and EA go/no-go is logged.

### Phase 4 — Year 3, endings, saga export (months 16–20)

All three endings (restored / escaped by ship / a stone in the highlands), trial permutations for replay, seeded runs, exportable saga text (the run printed as a saga chapter — shareable, Landnám-style). 1.0.

**Done when** all endings reachable and distinct, a full 3-year run holds 15–25 hours, and the exported saga of a real playthrough is good enough to post.

## Assets available (researched 2026-08; Fab prices not fetchable through this proxy — verify on Fab)

| Need | Asset | Notes |
|---|---|---|
| Terrain | [Iceland Large Heightmaps](https://www.fab.com/listings/dfc87314-8561-469f-97c6-acddfd81c22e) (135 × 8k, ~400 km² each, 2 m DEM) + North-region pack (151 more) | The actual island, purchasable. Pick one real valley and keep its name |
| Surfaces/rocks | Quixel Megascans | **Free for UE** — volcanic rock, moss, gravel all present |
| Farms/halls | [Medieval Nordic Village Environment](https://www.fab.com/listings/1b1acf17-f627-412c-b605-c8930c3698e4) (164 meshes, interiors) / Viking Village Environment Megapack (285, interiors, UE 5.6+) / [Modular Viking Village](https://www.fab.com/listings/a72879ea-577a-4125-bde8-25fdd51060cf) (178, Nanite/Lumen) | Any one covers all 18 farms; furnished longhouse also in [Medieval Buildings Vol. 1](https://www.fab.com/listings/18e7c2f6-166d-4808-8ed7-7f95018e7961) |
| Warriors/armor | [Viking Armour — MetaHuman ready](https://www.fab.com/listings/ef5111f6-4e28-4e43-a26f-b3c6060d3b10) + several rigged Viking packs | Modular helmets/armor/shields; MetaHuman itself is **free** |
| Farmers (the actual cast) | [Medieval Villager Vol. 1](https://www.fab.com/listings/1e09ca45-2c57-4a50-9cf9-02386d34ebb2) (UE5 skeleton, male/female, mix-and-match clothes), [Medieval NPC Pack v1](https://www.fab.com/listings/e6ff5eb2-f24a-4ee9-8004-fe1a80a496ed) | Solves the "packs only sell warriors" worry — vaðmál, not mail |
| Sky/weather/seasons | [Ultra Dynamic Sky](https://www.unrealengine.com/marketplace/en-US/product/ultra-dynamic-sky/) | Rain/snow/lightning, real sun position by latitude/date — at 65°N the light *is* the calendar. Verify price |
| Snow trails | [Snow Effects](https://www.fab.com/listings/dc272001-2090-4ff8-997c-4ec0adb0400d) (trail system: footsteps, animal tracks) / [Advanced Landscape Deformation](https://www.fab.com/listings/106fa7e9-c400-4dae-a31d-06e4dc8d721f) (Niagara snow/mud/sand) | Tracks in snow are a *pursuit mechanic* here, and it's purchasable |
| Horse | [Horse Starter Kit](https://www.fab.com/listings/121ad578-1980-4cfd-97cf-5ad64d0b1528) (69 anims, drag-drop) or Horse Animset Pro (200+ anims) + [Ultimate Horse System](https://www.fab.com/listings/0f1d9f24-9f5a-42dd-8934-5d6dbf1bc002) | Multiple mature options |
| Livestock/wildlife | [Animal Variety Pack](https://www.fab.com/listings/2dd7964c-a601-4264-a53d-465dcae1644c) / Complete Animals | Sheep for the farms, foxes for the fells |
| Combat base | [Melee ARPG Combat System](https://www.fab.com/listings/7fd1f6d2-9fbf-4ef7-856b-5fb84cd576bc) (bring-your-own-assets) or [Soulslike Framework](https://www.fab.com/listings/75455ba4-7407-45db-b24e-160712b9586c) stripped hard | Strip to: no combos, no sponges, wounds persist |
| Locomotion | Epic Game Animation Sample | **Free**, motion-matched |
| Dialogue | [Simple Dialogue (DataTable-driven)](https://www.fab.com/listings/d234ca2f-3ace-4369-a41e-b5f1ff092872) or [Dialogue Plugin](https://www.fab.com/listings/d432e314-10ab-4793-93b7-de7c3a56d704) | DataTable-driven fits the house style; no voice acting by design |

**Custom-work list:** shelter/trust + pursuit sims (pure C++ — the actual game, and home-turf work); saga-log port; Þing hearing scenes; winter-den authored content; melee strip-down and holmgang tuning; clothing gaps (some period pieces beyond the packs).

## Budget guess

Assets: mid hundreds of dollars (heightmaps + village pack + UDS + horse + snow + combat + characters — the most shopping of any candidate, and it buys the most). The spend is time: ~6 weeks to a gorgeous proof, ~9 months to a vertical slice a stranger can narrate back, ~16 months to EA candidate, ~20 to 1.0.

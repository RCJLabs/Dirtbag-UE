# DIRTBAG — Production Plan

*2026-08-15. Concept: [DIRTBAG.md](DIRTBAG.md) · Rival plan: [THREE-WINTERS-PLAN.md](THREE-WINTERS-PLAN.md) · Decision aid: [COMPARISON.md](COMPARISON.md)*

Format follows the Landnám ROADMAP convention: numbered phases, explicit **Done when** gates, a CURRENT MILESTONE marker. If Dirtbag wins the decision, this file *is* the starting roadmap.

---

## Pros & cons (the honest list)

**Pros**

1. **Nothing like it exists.** No serious 3D climbing life-sim has ever shipped. Jusant is a narrative traversal game with no sim; New Heights is a physics toy; the Steam tag is starving. First-mover in an identifiable niche beats tenth-mover in a loved one.
2. **The design is already proven.** The 2D game (v0.956, live) is a complete, balanced, player-tested spec. Almost every design question in this project has already been answered once — the 3D build inherits answers, not questions.
3. **Existing audience and identity.** Dirtbag's players, its store page, its name recognition carry over. "From the maker of Dirtbag" is a real launch asset.
4. **Authenticity is visible.** The dev climbs, and the 2D game's writing shows it. In a niche this specific, that's a moat — climbers can smell a non-climber's climbing game in one trailer.
5. **The risk fails fast.** The entire project hinges on one testable question (climbing feel), answerable in 4–8 weeks of gray-box for near-zero cost. Very few L-scope projects offer a cheap kill-switch this early.
6. **The pathfinder ships a real game.** The Hollow is revenue, audience, and tech validation inside year one, whatever happens to the flagship.

**Cons**

1. **Climbing feel can simply fail.** Hands-on-rock is one of the hardest animation problems in games; if the prototype never feels deliberate and weighty, there is no game. (Mitigated by the Phase 0 gate — but the risk is real.)
2. **Animation/IK is unfamiliar territory.** Every prior RCJ project's hard parts were *systems* — home turf. This one's hard part is Control Rig, FBIK, and contact quality: a new discipline with a real learning curve.
3. **It's the biggest scope on the shortlist.** Even rung 1 of the cut ladder is a year-plus of work after the pathfinder. The 2D game's content richness will constantly whisper "add me."
4. **Thinner asset support where it matters most.** Environments are solved (Megascans), but no one sells "great climbing hands" — the core loop is bespoke by definition.
5. **Niche ceiling.** Climbers are passionate but finite; the game must also land with sim/life-sim players who've never touched chalk. The 2D game's warmth suggests it can — but the 3D market test is unproven.

## Development phases

**>>> CURRENT MILESTONE: Phase 0 (not started — pending the Dirtbag-vs-Three-Winters decision) <<<**

### Phase 0 — Climbing-feel prototype (weeks 1–8)

The kill-switch phase. One gray-box 15 m wall, ~40 typed holds (crimp/sloper/pinch/pocket/jug), a character climbing via Control Rig + Full Body IK, a grip/pump model in plain C++ (standalone-testable, golden-vector style per the landnam-ue method), falls, shake-outs. No town, no art, no save system, no sound.

Buy first, build second: start from [Procedural Climbing with Control Rig](https://www.fab.com/listings/9a460f95-7079-48b6-b38f-e17d764d4f34) (FBIK placement, no canned animations — closest to our model), keep [Climb and Vaulting Component V2](https://www.fab.com/listings/a32f69ab-aefd-4d38-869f-b5414364aca1) and [Dragon IK](https://www.fab.com/listings/d3f8d256-d8d9-4d27-91c1-c61e55e984a6) as comparison/fallback. Budget one week purely for evaluating all three before committing.

**Done when** each of the five criteria has a written pass/fail with evidence, and a DECISION entry is logged in this file:
1. Moving between 10 holds feels deliberate, not floaty, within 5 seconds of picking up the controller (test on someone who isn't the dev).
2. A pumped-out fall reads as earned — the player saw it coming ≥2 moves out.
3. One route becomes meaningfully easier/harder by hold selection alone, no stat changes.
4. A reach change of ±10% (morphology stand-in) changes the beta on at least one sequence.
5. Frame budget holds with FBIK active on target hardware.

*Fail → close this file, open THREE-WINTERS-PLAN.md, carry the locomotion work over. No sunk-cost debate; the criteria were written first.*

### Phase 1 — The Hollow (months 3–8)

The shippable pathfinder: the slot-canyon ghost story (BRAINSTORM #7). Scope contract, binding: one canyon, 3–4 hours, climbing + narrative only — **no** economy, needs, factions, or save migrations beyond a bookmark. Story: descend, climb deeper, reconstruct the fallen climber's last season from what's left on the wall, finish (or refuse to finish) their line.

**Done when** it's on Steam at a modest price with store page, achievements, and a launch trailer cut from in-game footage. Secondary gate: the climbing stack (hold library, FBIK layer, pump model) lives in a plugin/module Dirtbag imports wholesale.

### Phase 2 — Core sim port (months 8–11)

The Dirtbag sim in plain C++, free of Unreal, tested against the 2D game as reference implementation: needs (cash/energy/hunger/skin), day clock, jobs and the odd-jobs board, gear with lean/wear, van wear, skills/training load/injury risk, weather + prime-window generation. All content in DataTables from day one. Save struct versioned with a migration registry from the first commit (SAVE_VERSION discipline, kept).

**Done when** a headless harness simulates 365 days deterministically from a seed, matching hand-checked expectations for economy drift, and adding a new route/gear item/job touches zero engine code.

### Phase 3 — MVP rung 1: one crag and the life around it (months 11–16)

Roadside Crag, the Lot (van + campfire + 2 neighbors), the dog, bouldering only. ~25 named routes + 3 open project lines (the FA pipeline live: clean → work → send → name → it enters the guidebook). Three partners with their own climbing careers. Skin/pump/window loop complete. Shifts and odd jobs for money. Sleep, bills, seasons.

**Done when** a full in-game season is playable start to finish with all systems live, and a new-player session reaches "I named my own route" inside 6 hours.

### Phase 4 — Rungs 2–4 (month 16 → )

In order, each a release: **(a)** town block (gym, gear shop, diner) + sport climbing (rope systems — the second animation mountain) + factions; **(b)** crags 2–3 with rock-type variety, trad, van travel with wear/breakdowns; **(c)** ethics arcs, sponsorship, legacy/generations. Early Access decision at (a).

## Assets available (researched 2026-08; Fab prices not fetchable through this proxy — verify on Fab)

| Need | Asset | Notes |
|---|---|---|
| Climbing base | [Procedural Climbing with Control Rig](https://www.fab.com/listings/9a460f95-7079-48b6-b38f-e17d764d4f34) | FBIK full-body placement, no canned anims — closest to our model |
| Climbing alt/reference | [Climb and Vaulting Component V2](https://www.fab.com/listings/a32f69ab-aefd-4d38-869f-b5414364aca1), Dynamic Ledge Climb System | Animation-driven; 20+ anims, foot IK, motion warping |
| General IK fallback | [Dragon IK Plugin](https://www.fab.com/listings/d3f8d256-d8d9-4d27-91c1-c61e55e984a6) | Universal IK if the specialist assets disappoint |
| Rock, cliffs, canyon | Quixel Megascans | **Free for UE.** The environment problem, solved |
| Ground locomotion | Epic Game Animation Sample | **Free.** Motion-matched base for everything that isn't climbing |
| Sky/weather/seasons | [Ultra Dynamic Sky](https://www.unrealengine.com/marketplace/en-US/product/ultra-dynamic-sky/) | De-facto standard; rain/snow, real sun position — *this is the prime-window renderer*. Verify price |
| Town | [Small Town America 3D Pack](https://www.fab.com/listings/ae122daa-b231-45d5-acee-23a8e87ef88c) | Gas station, motel, retail, houses, barn, ~100 props — almost suspiciously on-theme |
| Town extras | [Small Town Stores Modular](https://www.fab.com/listings/cfa3b6a2-427b-403a-9317-22450717cf53), [Modular Vintage American Diner](https://www.unrealengine.com/marketplace/en-US/product/modular-vintage-american-diner), [Gas Station](https://www.fab.com/listings/bdc9f384-a64d-4605-8cf4-3cb53771e1d7) | Lumen/Nanite-ready storefront and diner kits |
| Vehicles | [City Sample Vehicles](https://www.fab.com/listings/2909157b-ddfa-4cef-a925-69dc2467021f) | **Free**, 13 driveable — base for the van until a camper model is bought/kitbashed |
| The dog | [DOG](https://www.fab.com/listings/5a41c4ec-d50f-45e0-8384-c6dc905468c5) | 26 anims incl. sniff, howl, rest, sleep, angry |
| Dialogue | [Simple Dialogue (DataTable-driven)](https://www.fab.com/listings/d234ca2f-3ace-4369-a41e-b5f1ff092872) or [Dialogue Plugin](https://www.fab.com/listings/d432e314-10ab-4793-93b7-de7c3a56d704) | DataTable-driven option fits the house content style exactly |

**Custom-work list (the real budget):** hand-to-hold contact quality per hold type (the project's heart, months not weeks, spread across Phases 0–1); hold archetype library (~40 sculpts); pump/grip C++ model; rope/belay sim (Phase 4a); character look + small customization set; climbing sound design (chalk, breath, rubber on rock — WebAudio-synth instincts transfer, but this wants real foley).

## Budget guess

Assets: low hundreds of dollars total (the expensive part is nobody sells it). Software: UE free at this revenue scale. The spend is time: ~8 weeks to the go/no-go, ~8 months to a shipped game (The Hollow), ~16 months to Dirtbag rung 1.

# DIRTBAG vs THREE WINTERS — Decision Aid

> **DECIDED (2026-08-15): DIRTBAG.** Evan chose the Dirtbag reimagining, with a pivot that resolves this file's central tension: climbing is a minigame-driven watched session ("2D minigames, 3D staging"), not physical climbing — so the tech risk this comparison hinged on was designed out rather than prototyped out. Three Winters is shelved as the named fallback/next game. Live roadmap: [/ROADMAP.md](../ROADMAP.md).

*2026-08-15. Concepts: [DIRTBAG.md](DIRTBAG.md) · [THREE-WINTERS.md](THREE-WINTERS.md). Full production plans: [DIRTBAG-PLAN.md](DIRTBAG-PLAN.md) · [THREE-WINTERS-PLAN.md](THREE-WINTERS-PLAN.md) — each written in the Landnám ROADMAP idiom (phases, "Done when" gates), ready to be the real roadmap of whichever wins.*

## The one-sentence version

**Dirtbag risks failing fast on tech; Three Winters risks failing slow on content** — and for a solo dev, fast-failing risk is the kind you want, because you find out in week 6 instead of month 14.

## Side by side

| | **DIRTBAG** | **THREE WINTERS** |
|---|---|---|
| **Risk profile** | Concentrated: one question ("does climbing feel good?"), answerable in 4–8 weeks of gray-box | Diffuse: content volume, shows up mid-project; mitigated by hard budgets, never eliminated |
| **Time to first shippable thing** | ~8 months (The Hollow, a whole small game) | ~6 months (vertical slice — but a slice, not a product) |
| **Time to the "real" game** | Long (M2 at months 8–14 is rung 1 of 4) | Moderate (fixed 3-year run structure bounds it; EA at ~month 14) |
| **Design maturity** | Highest possible — a shipped, balanced 2D game is the spec | High — sagas as source, Landnám's systems as parts, but the game itself is new |
| **Code/design reuse** | 2D Dirtbag as reference implementation for every sim system | Landnám sim logic (feud, wergild, winter, events) + landnam-ue C++ method + hex lib |
| **Fab coverage** | Rock solved (Megascans); climbing systems exist as scaffolding; **hand-to-hold quality is real custom work**; dog purchasable | Best on the board: real Iceland heightmaps, 3 modular village packs w/ interiors, MetaHuman-ready armor, combat frameworks |
| **Custom-work center of gravity** | Animation/IK (unfamiliar territory) | Systems in C++ (home territory) |
| **Market position** | Unclaimed genre; existing Dirtbag audience carries over; "you climb, it shows" authenticity | Saga-realism RPG is untapped (everyone does mythology, nobody does the sagas); Skyrim-comparison risk cuts both ways |
| **What week 6 looks like** | You *know* — the prototype either feels right or it doesn't | Terrain and a farm look gorgeous (all purchased parts); the hard questions are still ahead |
| **What month 14 looks like** | Rung-1 MVP playable; The Hollow already shipped and selling | EA candidate if budgets held; the danger month if they didn't |
| **Failure mode** | Prototype says no → weeks lost, fall back with confidence | Grind realization → months lost, morale damage |
| **Heart** | The game he's been making all along | The game his flagship universe is asking for |

## Recommendation

**Start Dirtbag's Stage A gray-box prototype (4–8 weeks) with the written go/no-go criteria in DIRTBAG.md §5 — and treat Three Winters as the funded, named fallback, not a discarded option.**

Reasoning:

1. The prototype is the cheapest information purchase available: 4–8 weeks buys the answer to the only question that decides between these two games.
2. If **go**: proceed to The Hollow → Dirtbag, with a shipped game inside the first year. Three Winters keeps maturing in the background (Landnám development feeds it directly — every feud/winter/event system improved there is Three Winters R&D for free).
3. If **no-go**: pivot to Three Winters having lost weeks, not months, carrying zero sunk-cost pressure — and the movement/traversal work from the prototype isn't even fully wasted there.
4. Either way, no decision made today gets regretted, because today's decision is only: *run the experiment.*

**Decision date:** end of Stage A, criteria in hand. Put it on the calendar; a prototype without a decision date becomes a hobby.

## Update (same day): this is an ordering, not a fork

With both production plans written, note what the two Phase 0s actually are:

- **Dirtbag Phase 0** (weeks 1–8): gray-box climbing prototype → written go/no-go.
- **Three Winters Phase 0** (weeks 1–6): assemble purchased parts → screenshot test + travel-model decision.

Both are cheap. Both produce information, not throwaway work (Dirtbag's locomotion carries to Three Winters; Three Winters' Phase 0 is 90% shopping). Running Dirtbag's first delays Three Winters by *at most two months* — and if the climbing prototype fails its gate, the heart gets its viking RPG anyway, guilt-free, with the brain fully on board. The only genuinely bad outcome is running neither while deciding.

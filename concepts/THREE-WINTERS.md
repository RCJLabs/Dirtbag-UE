# THREE WINTERS — Concept Doc (viking saga RPG)

*Finalist deep-dive, 2026-08-15. Companion: [DIRTBAG.md](DIRTBAG.md), [COMPARISON.md](COMPARISON.md).*

> At the Þing you are sentenced to lesser outlawry for a killing that was arguably fair. Three years. Inside the district, anyone may kill you without penalty — and the wronged family intends to. Survive to the Þing that restores you. Or don't.

---

## 1. Vision & pillars

Skyrim's landscape-wandering with the fantasy removed and the sagas put back. No dragons, no fireballs, no chosen one: weather, distance, hunger, and men with a legal right to your life. Grettir's saga and Gísli's saga are the design document — both are literally stories of outlaws surviving years in this exact landscape, and nobody has made this game.

**Pillars, in priority order:**

1. **The sentence is the antagonist.** Not a villain — a legal status. The game is a countdown (three years) against a pursuit that never fully sleeps.
2. **Shelter is the resource.** Food and warmth come from *people*, and every person who helps you risks outlawry themselves. The real inventory is a network of farmers who owe you, fear you, or loved your father.
3. **Combat is rare, short, and lethal** — and every killing you commit extends someone's reason to hunt you. The best fight is the one talked, paid, or walked away from.
4. **Saga realism.** Wergild, guest-right, the Þing, bad blood with a name on it. Landnám's vocabulary and chronicle voice, at the scale of one man.
5. **The chronicle writes itself.** Every season ends with a saga-log entry in past-tense chronicle prose; the full run prints as *your* saga. (Landnám's proven trick; its Phase-7 notes already observe a narrating skald is nearly free content.)

## 2. Player fantasy & core loops

**The frame.** Character creation is the trial: who you killed, why it was arguably fair, and what your family could pay — these choices set your starting enemies, sympathizers, and wergild debt. Then the district is closed to you, and the interior is open.

**Seasonal loop (the heartbeat):**
- **Summer:** move. Work under a false name at outlying farms (haying, herding, smithing — labor mini-loops kept light), build the shelter network, scout winter dens, dodge the hunters who ride the paths between farms.
- **Autumn:** the gamble. Cache food, choose the winter den — a sympathetic farm's outbuilding (warm, but you're betting their courage), a cave in the highlands (safe, but the cold is Landnám's winter), or an island in a lake (Grettir's actual solution).
- **Winter:** den up. The game contracts to people-in-a-hall: long-house evenings, stories, the farmer's son who suspects, the daughter who doesn't care, cabin fever, and the one set of tracks in the snow that means everything. "The winter is the boss" — ported intact.
- **Spring:** the hunt resumes with new information. Anyone who saw you last year talked, or didn't.

**The hunt (the pressure system).** The wronged family's hunters are a small cast of *named men* — not spawns — with homes, routes, informants, and their own doubts. Kill one and his brother takes it up with better cause. Elude one for a year and he starts arguing at the Þing to let it go. The pursuit is a simulation you can read and manipulate: rumors, sightings, paid silence.

**Endpoints.** Each summer, the Þing convenes and your case can shift: sympathizers argue mitigation, wergild can be raised and offered, hunters can overreach and lose standing. Endings: **restored** (sentence served or settlement paid), **escaped** (take ship, exile — a quieter, sadder win), or **a stone in the highlands with your name on it.** All three print a complete saga.

## 3. Systems map

**Reused from Landnám (design and, partly, code):**
- World, vocabulary, tone: Þing, wergild, feuds, chronicle voice — same universe, one man deep.
- **Feud/wergild logic:** Landnám already simulates quarrels that escalate to knives unless paid or argued out; this game *is* that system promoted to the spine.
- **Winter survival math:** cold, food, the vague long-range forecast (deliberately imprecise so stockpiling is a gamble, not arithmetic).
- **Event-card content style:** 100+ authored events as data (Landnám shipped 102), each a page of chronicle prose with 2–3 choices — this is the main content format and Evan's proven fastest content pipeline.
- **landnam-ue groundwork:** the C++-sim-core-free-of-Unreal method, parity harness against golden vectors, deterministic named RNG streams, DataTables for all content, hex/A* library if overworld travel is abstracted (see below).

**New systems (the honest new-build list):**
- **Shelter/trust network:** per-household disposition (fear/sympathy/greed), risk tolerance, and memory. This is the game's real RPG stat sheet.
- **Pursuit simulation:** named hunters with routes and information flow.
- **Dialogue:** menu-driven, saga-terse. No voice acting — text fits the chronicle register and the budget.
- **Melee:** short, lethal, readable. Buy a framework (below), strip it brutally — no combos, no health sponges; wounds that persist and infect.
- **Stealth-lite:** sightlines, tracks in snow, false names. Systemic, not Thief — being *recognized* is the fail state, not being seen.

## 4. Content-volume risk plan (the centerpiece)

Open-world RPGs kill solo devs with content. The defense is structural, decided now:

1. **One district, not a continent.** A single fjord-and-highland district, ~25 km² of authored space on real Iceland heightmap terrain. Skyrim is the *feel* reference, not the *scale* reference.
2. **Hard content budget, written into the roadmap like Landnám's phase gates:**
   - **~18 farms/households**, each built from a **household template system** (roles: head, spouse, heir, hands; dispositions; one authored secret each) — data-driven like Dirtbag's caravan neighbors, hand-touched, not hand-built.
   - **~6 named hunters** across the three years (the entire antagonist cast).
   - **~120 event cards** (40/year) + **~30 winter-den scenes**.
   - **3 authored feud arcs** (yours + two neighbors' you can be drawn into).
3. **The fixed three-winter structure is the budget.** A run is 3 in-game years, ~15–25 hours. Not endless. Replayable via trial setup + seeded variation, Landnám-style.
4. **Winter contracts the world.** A third of each run happens in one location with a small cast — the cheapest content in the game is also its emotional core.
5. **Travel may be hybrid:** real third-person traversal inside the valley; **abstracted travel (Landnám's hex layer) for long crossings** if open-world streaming costs too much. Decide at prototype; the hex library already exists in C++.

## 5. MVP scope & cut ladder

1. **One valley, one winter, one feud.** Vertical slice: trial → one summer of shelter-building among ~6 farms → autumn den choice → one full winter → spring Þing hearing. Every core system present once.
2. **+ Full district (18 farms), year 2, the hunter cast complete.**
3. **+ Year 3, all endings, feud arcs 2–3, ship-escape ending.**
4. **+ Replayability pass:** trial permutations, seeded events, saga export.

## 6. Fab shopping list (verified 2026-08)

**Exists — buy, don't build:**
- **Terrain:** [Iceland Large Heightmaps](https://www.fab.com/listings/dfc87314-8561-469f-97c6-acddfd81c22e) — 135 real 8k heightmaps from 2 m-precision DEMs (a north-region pack with 151 more also exists). The *actual island*, purchasable. Surfaces via free Megascans.
- **Architecture:** three strong modular Viking packs with interiors — Viking Village Environment Megapack (285 meshes, UE 5.6+), [Medieval Nordic Village Environment](https://www.fab.com/listings/1b1acf17-f627-412c-b605-c8930c3698e4) (164 meshes), Modular Viking Village (178 meshes). One of these covers every farm in the district.
- **Characters:** [Viking Armour — MetaHuman-ready modular set](https://www.fab.com/listings/ef5111f6-4e28-4e43-a26f-b3c6060d3b10) plus several rigged warrior packs; MetaHuman for faces if realistic, or a stylized pack (Synty Vikings) if not.
- **Combat base:** [Melee ARPG Combat System](https://www.fab.com/listings/7fd1f6d2-9fbf-4ef7-856b-5fb84cd576bc) (designed to take your own assets) or the [Soulslike Framework](https://www.fab.com/listings/75455ba4-7407-45db-b24e-160712b9586c) stripped down. Locomotion from Epic's free Game Animation Sample.
- **Livestock/wildlife:** sheep, horses, dogs all in standard animal packs (Animal Variety / Complete Animals).

**Custom work:**
- The shelter/trust and pursuit sims (pure C++, the actual game).
- Saga-log generation (port of Landnám's chronicle system).
- Winter: snow accumulation/tracks (RVT-based snow is well-trodden UE territory, but it's ours to tune — tracks are a *pursuit mechanic*, not decoration).
- Period-correct clothing beyond armor (most packs skew warrior; farmers need vaðmál, not mail).

## 7. Risks & mitigations

| Risk | Odds | Mitigation |
|---|---|---|
| Content volume balloons | The big one | Hard budget above; template households; fixed 3-year structure; winter contraction |
| "Skyrim but empty" first impression | Real | Sell it as a survival-saga, not an open-world RPG; density over acreage (25 km², 18 farms is *dense*) |
| Melee feel mediocre | Moderate | Combat is rare by design; a 6-fight game needs 6 great fights, not a combat system |
| Systemic pursuit reads as random | Moderate | Named hunters with visible logic; rumors surface *why* they came |
| Stealth/social systems interact badly | Moderate | Vertical slice (MVP 1) exists to find this in month 4, not month 14 |

## 8. Rough milestones

- **M0 (weeks 1–6):** terrain + one farm + walk/ride/weather; decide real-vs-hybrid travel. (Cheap — all purchasable parts.)
- **M1 (months 2–6):** vertical slice — one valley, one winter, one feud, playable end-to-end.
- **M2 (months 6–14):** full district, hunter cast, year 2.
- **M3 (months 14–20):** year 3, endings, saga export. Early Access candidate at M2.

## 9. Open questions for Evan

1. Third-person confirmed? (Recommend yes — the landscape and the hall scenes are the spectacle.)
2. Realistic (MetaHuman + Megascans, Iceland heightmaps shine) vs stylized (faster, safer)? This game pulls realistic harder than Dirtbag does.
3. Is this canonically the Landnám universe (same steading names a few generations on) or adjacent? Canon link is free marketing between your own games.
4. How literal is the labor? (Haying/herding as mini-loops vs. abstracted work-for-shelter choices. Recommend abstract-first; add hands later if the slice wants them.)
5. Combat perspective on the 6-fight promise: fixed authored duels (holmgang!) vs systemic encounters? Holmgang-as-boss-fight is very on-theme.

# Unreal Game 2 — Brainstorm

*20 ideas for the next RCJ Labs game, built in Unreal Engine. Written 2026-08-15.*

Four slots were mandated: a reimagining of **Dirtbag**, a **viking RPG**, a **survival game**, and a **shooter RPG**. The other sixteen were chosen to fit the design DNA that runs through Dirtbag, LVDVS, Sandbagged, and Landnám:

- **Named people, never units.** Permadeath, chronicles, relationships. If a system would read the same with numbered slots instead of people, it's the wrong system.
- **The environment is the antagonist.** The route, the winter, the arena, the storm. You don't kill anything; you top out, or you hold on, or you get down alive.
- **Ethics are gameplay,** with consequences that resurface seasons later, not one-off choices.
- **Legacy and generations.** Aging is load-bearing; runs end and hand something to the next one.
- **Solo-dev scope discipline.** Sim logic pure and testable, content data-driven, one strong idea polished over ten weak ones.

**A note on Fab before the list.** Quixel Megascans is free for Unreal use, which means photoreal cliffs, boulders, canyon walls, alpine terrain, and forest floors cost nothing — the environment problem for every outdoor idea below is largely solved. Deep, reliable Fab categories: Viking/medieval villages, dungeons and caves, Roman architecture, weapon packs, small-town Americana, oceans and boats. The genuinely thin areas that need custom work or careful shopping: **climbing animation and hand-to-hold IK** (climbing-system assets exist as starting points, but send-quality hands are custom), **sled dogs**, and **wagon-era Americana**. Scope ratings: **S** = months, **M** = around a year, **L** = multi-year unless cut hard.

---

## The mandated four

### 1. DIRTBAG *(the reimagining)*

**Pitch.** The climbing life-sim in first/third person: one American West valley, a town, the Lot, your van, and a handful of crags you can walk to the base of. Live out of the van, work shifts, and push your grade from gym plastic to a first ascent with your name on it.

**What carries over from 2D — the crown jewels, each better in 3D:**

- **Prime-conditions windows** stop being a stat modifier and become the day's staging: light moving across the wall, the shade line you wait in, friction you can feel through the controller. Saving the project burn for the 4pm window is a real 3D loop no climbing game has built.
- **Skin and pump** stop being meters and become the body: taped tips, a visible forearm shake-out at the jug, the decision to settle into a hold or move now.
- **Ascent style as the score.** Onsight > flash > redpoint > sent, and the cheapening actions are *visible* — tick marks and chalk on the holds tell you someone's been here, and yours tell everyone else.
- **The first-ascent pipeline.** Open lines at every crag that you clean, work, send, and name — then they're in the guidebook and other climbers show up to try them.
- **The van as a wearing machine** you drive between crags and can hear failing. **Morphology** becomes literal reach and body position. **The dog** still finds you at the crag.

**The central design call.** The 2D game rolls send-odds dice; the 3D game shouldn't. Keep the odds model as *the body's honest limits* — fatigue, skin, conditions, and morphology gate what you can hold and for how long — and let player execution do the rest. That preserves everything that makes Dirtbag Dirtbag (prime windows, pump management, aging) while making climbing a game you play, not a check you pass.

**Fab.** Megascans covers rock and terrain outright. Small-town packs, diners, camper vans, and campground props all exist. The gap is the climbing itself: buy a climbing-system asset as scaffolding, then budget serious time on hand IK and hold-contact animation — this is the project's tech heart.

**Scope: L** — but honestly cuttable to *one crag, one town block, the van, the dog* for a first release, with the 2D game as the complete design document.

---

### 2. THREE WINTERS *(the viking RPG)*

**Pitch.** A third-person saga RPG. At the Thing you are sentenced to lesser outlawry for a killing that was arguably fair. Three years. Anyone may kill you without penalty inside the district; the wronged family intends to. Survive to the Thing that restores you — or don't.

**Core loop.** Skyrim's landscape-wandering across an Iceland-scale interior, but low fantasy and saga-real: no dragons, no fireballs, just weather, distance, and men. Shelter is the resource — farmers who take you in risk their own necks, and each has a price, a grudge, or an old debt to your father. Combat is rare, short, and lethal; most encounters are talked, paid (wergild), or walked away from, and every killing extends someone's reason to hunt you. Seasons turn: summer you move and work for shelter under a false name, winter you den up with whoever will have you and the game becomes people in a hall.

**Why it fits.** Landnám's world, vocabulary (Thing, wergild, bad blood with a name on it), and chronicle voice port directly — this is the same universe at the scale of one man. Grettir's and Gísli's sagas are the design document, and nobody has made this game.

**Fab.** Viking/Norse is one of the deepest categories on Fab — halls, farms, clothing, weapons. Iceland terrain via Megascans and a heightmap tool. Character animation from standard humanoid packs; melee needs care but is well-trodden.

**Scope: M–L.** The world can be one district, not a continent, and the saga structure (three winters, fixed ending) bounds it naturally.

---

### 3. WHITEOUT *(the survival game)*

**Pitch.** First-person mountaineering-disaster survival. A storm shatters your expedition above Camp 3 on an 8000m peak. The summit no longer matters. The game is the descent.

**Core loop.** Run-based, several in-game days per run. Cold is the boss and it is arithmetic you can feel: layers, wetness, wind, work rate, frostbite tracked by digit. Route-finding in whiteout, fixed lines that may or may not hold, rappel anchors you build from what's left, dead climbers' gear you must decide whether to take, living climbers you must decide whether to help — every act of decency spends your own margin, and the game never tells you the exchange rate. Reach base camp with what's left of your hands, or become gear for the next run's storm.

**Why it fits.** "The winter is the boss" made literal, at pitch-black stakes. Sandbagged's route-as-opponent and Landnám's vague-forecast gambling both feed straight in. It's the mandated survival slot but also secretly a climbing game — the tech (ice axe, rope, crampons on terrain) is adjacent to Dirtbag's needs.

**Fab.** Alpine/snow environments and blizzard VFX exist; Megascans covers rock and ice surfaces. Gap: ice-axe/rope/rappel animations are custom-ish, but first-person hides most of the body.

**Scope: M.** One mountain, one route down, run-based structure. Contained and very shippable.

---

### 4. THE LONG TRAIL *(the shooter RPG)*

**Pitch.** Post-collapse America, sixty years on. A 2,000-mile national scenic trail — the blazes still on the trees — is now the safest road through the ruins, and you walk it as a courier: mail, medicine, seeds, and secrets between the settlements that grew up in the old trail towns. Fallout's tone, but *walking north is the quest*.

**Core loop.** Hike sections, manage pack weight as the honest inventory system (ammo is heavy; so is kindness), resupply in trail towns with their own politics, and fight only when the trail forces it. A scarce-ammo economy where every shot is a decision and a fired gun is a broadcast of your position. Factions with genuinely opposed values, Dirtbag-style: trail wardens who keep the way open for everyone vs. hoarders who tax it, plus the towns that just want the mail to come. Trail registers — notebooks at shelters — carry lore, rumors, and the fates of NPCs who walked ahead of you.

**Why it fits.** Fallout's wasteland warmth plus thru-hiker culture (shakedowns, trail names, trail angels — the game gives *you* a trail name the way Dirtbag's scene names your crew). Chaptered by trail section, so it ships in slices like Sandbagged's acts.

**Fab.** Lyra plus gun packs solve shooter feel out of the box; Megascans forest; ruin/overgrowth kits are plentiful. Least asset risk of any idea this size.

**Scope: L, honestly — M** if the first release is one 300-mile section.

---

## The climbing family

### 5. THE WALL

**Pitch.** One 1,000-meter big wall is the entire game. A multi-day ascent: lead pitches, build anchors, haul the bag, sleep on a portaledge with the void under the fabric, watch the storm window close, decide to push or bail.

**Core loop.** Dirtbag's expedition layer (pitches, anchors, bivvies, high points, bail decisions), fully embodied. Days matter: water and food are counted, skin degrades pitch by pitch, weather is forecast vaguely at range and precisely too late. Retreat is always possible and always costs — bail from pitch 18 and the game remembers your high point for the next attempt, Dirtbag's siege-projecting system at the scale of one monolith.

**Why it fits.** Jusant's intimacy with sim teeth. One hero asset, total polish — the best-scoped *serious* climbing idea on this list, and everything built for it (hold interaction, rope systems, weather) is a straight deposit into Dirtbag 3D.

**Fab.** One sculpted face plus Megascans surfaces; minimal world beyond the wall and the meadow below it.

**Scope: S–M.**

### 6. SETTER'S GYM

**Pitch.** A climbing-gym tycoon where route-setting is a first-person creative tool. Bolt holds to the wall with your own hands, forerun your own sets, then watch the members flail on your crux from the mezzanine.

**Core loop.** Management layer: memberships, comps, reset schedules, coaching programs, the culture of the gym (a board-bro gym and a kids-program gym are different businesses). Creative layer: physically set routes — hold choice, spacing, style — and the sim populates them with members whose morphologies and skills read your set differently, Dirtbag's body-vs-route model pointed the other direction. Share set walls as codes, his established save-code habit turned into UGC.

**Why it fits.** Dirtbag already contains gym ownership as endgame content; this promotes it to the whole game. The route-is-the-opponent thesis, but now you're the route.

**Fab.** Warehouse interiors and gym equipment exist; climbing holds are trivial modeling. Crowd animation is the main lift.

**Scope: M.**

### 7. THE HOLLOW

**Pitch.** A short atmospheric ghost story grown from Dirtbag's secret crag: a slot canyon that isn't on any map, headstones at the base of the wall, and a climber who fell decades ago and never topped out. Climbing as traversal in a 3–4 hour narrative horror.

**Core loop.** Descend into the canyon, climb deeper, piece the dead climber's story from what they left on the wall — fixed gear rusting in the cracks, a journal in a tin, chalk that shouldn't still be there. The climbing is deliberate and unhurried; the dread comes from the place, not jump scares. It ends on the line they never finished, and whether you finish it for them.

**Why it fits.** Dirtbag's Halloween ghost, The Hollow, and the graveyard canyon already exist as canon. The strategic value is bigger than the game: it ships in months and **pathfinds the entire climbing tech stack** — hold interaction, hand IK, rock assets, canyon lighting — for idea #1, the way landnam-ue's hex grid pathfound Landnám.

**Fab.** Canyon and rock via Megascans; one location; horror ambience packs are abundant.

**Scope: S.**

### 8. FELL

**Pitch.** A mountain-running / fastest-known-time game. The mountain doesn't fight back with monsters; it fights back with 9,000 feet of vertical and your own pacing.

**Core loop.** Movement mastery: pacing against a redline, gait choice on scree and talus, line choice through terrain, fueling before the bonk hits (it arrives twenty minutes after the mistake, like real bonking). Between attempts, a light training-block life-sim — Dirtbag's training load and burnout systems — decides what body you bring to the next window. Race the ghosts of your own splits; weather windows decide when a record attempt is even possible.

**Why it fits.** The running Dirtbag: same body-as-resource design, same conditions-window gambling, radically smaller content needs. Nobody has made the serious FKT game.

**Fab.** Terrain is the easy part; the entire game lives in animation and camera feel, which is the risk.

**Scope: S–M.**

---

## The Norse family

### 9. WHALE ROAD

**Pitch.** A knarr sailing-and-trading sim on the Norse Atlantic: Norway → Faroes → Iceland → Greenland → Vinland. No GPS, no map cursor. Navigation itself is the antagonist.

**Core loop.** Honest sailing — wind, leeway, reefing before the squall, dead reckoning by sunstone, landmark, and the color of the water. Cargo speculation across ports that only rumor connects (walrus ivory west, timber east, and the prices moved while you crossed). The crew are named men who eat, freeze, grumble, and mutiny — Ludus's men-are-not-units at sea, and every man lost is a household you answer to at home.

**Why it fits.** Landnám opens with six people stepping off a knarr; this is the game about the knarr. Sailwind proved the audience for honest small-boat sailing; nobody has done it Norse.

**Fab.** Viking ship models and UE water/ocean systems both exist and are good. Ocean rendering is a solved problem in UE5.

**Scope: M.**

### 10. BARROW

**Pitch.** A low-fantasy dungeon crawler in saga Iceland. The dungeons are burial mounds. The loot is real, and so is the anger of the dead.

**Core loop.** Episodic: each barrow holds **one named dead man**, and you can't open his grave until you've learned his story — asked the old women, read the rune stone, found where the feud that killed him ended. Inside: darkness, dread, and *maybe* a draugr — fights are rare, terrible, and mostly avoidable if you learned enough. The loot is cursed in **socially real** ways: the heirloom sword sells high until the dead man's living family recognizes it at the Thing, and now you have a feud. Wergild, reputation, and whether you rebury what you took are the actual game.

**Why it fits.** Skyrim's draugr crawls with Landnám's social physics wrapped around them — the consequence system is the curse. Episodic structure ships in slices.

**Fab.** Modular dungeon/cave/tomb sets are among the deepest categories on Fab; one barrow interior kit goes a long way.

**Scope: S–M.**

### 11. SWORN BROTHERS

**Pitch.** A Viking warband tactics RPG, XCOM-shaped: a dozen named men for hire in the seasons between raids. Turn-based battles won by the shield wall, lost by the man who breaks it.

**Core loop.** Overworld contracts (escort the bride, hold the bridge, burn the hall — each with an employer and a wronged party), turn-based battles on hex or grid where **shield-wall adjacency is the core mechanic** — a warrior with a shoulder-mate is harder to kill, two harder still, and the wall shatters when a link falls. Landnám's battle system, already built, measured (formation beats brawling 32–60 vs 29), and proven. Permadeath with wergild owed for men you spend; camp scenes between contracts where grudges form and sworn oaths bind.

**Why it fits.** The single best systems-to-content ratio on this list: the tactical core exists in tested TypeScript, turn-based needs far less animation fidelity than action games, and Ludus proved he can make a roster of mortal men carry a whole game.

**Fab.** Modular Viking character packs plus standard melee animation sets; battlefields are small handcrafted maps.

**Scope: M.**

### 12. BLOODLINE

**Pitch.** A generational steading sim. One farm, five generations, first person. You play the heir, every time, and a parent's death is a save point.

**Core loop.** Run the steading through years that pass for real: fields, stock, storms, winters. Marry — a negotiation between families, not a mini-game — and raise the children one of whom you will become. Feuds with the neighboring farms are inherited liabilities carried by name; the house physically accretes rooms, scars, and graves over decades, the landscape's memory made visible. Each generation opens with the funeral and the reading of what you're owed and what you owe.

**Why it fits.** Landnám's parking lot literally contains "bloodline/generation play"; Dirtbag already shipped a legacy/forebear system. This grows both into the whole game — Crusader Kings' generational pull at the scale of one household you can walk through.

**Fab.** Norse or 1800s-American setting both fully covered (farm buildings, livestock, period props).

**Scope: M–L.**

---

## The American frontier / wilderness family

### 13. FREETRAPPER

**Pitch.** An 1820s Rocky Mountain fur-trapper survival RPG. Run traplines through country that wants you dead, and once a year come down to Rendezvous to sell, drink, resupply, and hear who didn't make it.

**Core loop.** Seasonal: set and run traplines (reading sign, watching weather, hauling weight), survive winters The Long Dark would respect, then the Rendezvous — the year's one town, a two-week social hub where prices, gossip, hiring, and feuds happen. The RPG spine is relations with native bands and rival brigades: territory, trade, marriage, and wrongs that compound. The sim is honest about the economy's brutality and its arc — beaver prices are falling, the streams are trapping out, and the game's long ending is the fur trade's actual collapse.

**Why it fits.** Survival + sim + RPG in one, environment-as-antagonist, an economy with a built-in elegy. Red Dead's country with Landnám's teeth.

**Fab.** Megascans wilderness, frontier props, black-powder weapons, and wildlife packs all exist.

**Scope: M–L.**

### 14. THE CROSSING

**Pitch.** Oregon Trail, embodied. You captain a wagon train 2,000 miles in first/third person, and the trail is a chronicle of named families.

**Core loop.** Daily marches with real decisions (pace vs. oxen, ford vs. ferry vs. wait), rationing, breakdowns, sickness that takes people with names and children. The nightly camp council is the social core: grievances, factions among the families, and the authority you spend or hoard. Permadeath chronicle throughout — the game remembers every grave by name and mile marker, and the ending tallies who stepped off in the valley.

**Why it fits.** Landnám's DNA on American soil — a fixed-length survival run with named people, where the win condition is "anyone alive." The saga log becomes the train's diary.

**Fab.** Prairie and mountain terrain are easy; wagons and oxen exist but the category is thinner than most — moderate asset risk.

**Scope: M.**

### 15. HOTSHOT

**Pitch.** A wildland fire crew sim: half management, half field ops, against a fire that reads wind and slope better than you do.

**Core loop.** Season layer: build and keep a 20-person crew (quals, fitness, pay, burnout — the roster is Ludus's), take contracts, manage fatigue across a fire season that keeps getting longer. Field layer: cut line, run burnout operations, and above all keep escape routes and safety zones current, because the fire is a live simulation that crowns, spots across your line, and turns on a wind shift. The subject, like Mountain Rescue's, is who keeps showing up.

**Why it fits.** The purest environment-as-antagonist on the list, and a management/field split he's built twice already.

**Fab.** Forest assets trivial; the risk is tech, not assets — a credible fire-spread sim plus Niagara smoke/flame at scale is the hard part.

**Scope: M–L, tech-risky.**

### 16. THE DISTRICT

**Pitch.** A park-ranger life-sim: one national park district, four seasons a year, for years. Firewatch's love of place with real sim systems under it.

**Core loop.** Systemic days from a duty roster you influence but don't control: trail work, wildlife encounters (the bear that got into the coolers is a named recurring character), drunk visitors, lost hikers becoming SAR callouts, poachers, fee-booth small talk that accretes into relationships — and the quiet, which the game treats as content, not absence. Multi-year timescale: seniority, transfers you can refuse, the park changing under budget cuts and fire seasons.

**Why it fits.** Dirtbag's town-and-day loop relocated to public land; the salaried-job-as-trap theme inverted into a job worth keeping.

**Fab.** Landscape, cabins, pickup trucks, Americana — all deep categories.

**Scope: M.**

### 17. GEE & HAW

**Pitch.** Long-distance dog sledding: a 1,000-mile race, and the kennel years between races. Fourteen dogs, every one named, aging, with quirks.

**Core loop.** Race layer: checkpoints, mandatory rests, run/rest strategy in deep cold, open water, moose on the trail at 2am. Kennel layer: breeding, training, and the roster decisions of which aging leader gets one more race — Dirtbag's "the dog lands hard" emotional register multiplied by a team, with Ludus's roster management underneath. The dogs' trust in you is earned per-dog and spent per-decision.

**Why it fits.** Named mortal teammates, environment as antagonist, a management/execution split — his entire toolkit, and an untouched genre.

**Fab.** The honest problem: sled dogs and dog-team animation are genuinely thin on Fab, and sled physics is custom. Highest asset risk on the list.

**Scope: M, asset-risky.**

---

## The management / spectacle family

### 18. LVDVS 3D

**Pitch.** The lanista, embodied. Walk your own gladiator school in first person; on fight days, watch from the box — because the arena is the antagonist, and the ludus is the game.

**Core loop.** LVDVS's proven systems port wholesale: buy men, train them, manage defiance that scales with talent, navigate patrons and rival houses, and face the rebellion arc when it comes. What 3D adds is presence: walking the yard at dawn and reading morale in how men train, sitting the pulvinus while a man you named fights below — spectating with real stakes, which almost no game does. Fights stay watched, not driven; your inputs are the years of decisions that got him there.

**Why it fits.** The most de-risked idea here — a mature, balanced design (v3.26, Monte-Carlo tested) that only needs a renderer worthy of it.

**Fab.** Roman architecture, arenas, gladiator gear, and crowd systems are all plentiful.

**Scope: M–L** (the fight choreography, watched or not, is real animation work).

### 19. BASECAMP

**Pitch.** An 8000m expedition management sim. You are the expedition leader, not the summiteer. The mountain is climbed from a folding table at 5,300 meters.

**Core loop.** Logistics (oxygen, ropes, camps, permits), acclimatization rotations, and forecasts that are deliberately vague at range and sharpen too late — Landnám's winter-forecast gambling at altitude. Sherpa relations, load fairness, and pay are a real ethics arc, not flavor. It all converges on the one weather window: who goes up, in what order, and — the mechanic the whole game exists for — who you turn around, over the radio, 800 meters from the summit they paid you for.

**Why it fits.** Ludus's men-are-not-units plus Landnám's winter-boss, mostly UI over one mountain — enormously buildable, and pairs with WHITEOUT as two lenses on the same peak.

**Fab.** One mountain, tents, and interface; the lightest asset load on the list.

**Scope: S–M.**

### 20. THE SIEGE OF ______

**Pitch.** Turn-based siege-defense tactics: you hold one named place — a hall, a bridge, a mountain pass — against everything one terrible winter sends, with the people you have and no relief coming.

**Core loop.** Rogue-lite runs, one handcrafted map each. Between assaults: repair, ration, treat wounds that persist, and settle the quarrels of named defenders whose nerve is a stat that breaks. During assaults: Landnám's raid-defense grown into the whole game — shield wall in the breach, palisades that force attackers to climb one at a time, the hall at your back and nowhere behind it. Each run's fall or survival writes a chronicle entry the next run inherits as legend.

**Why it fits.** Landnám's most-praised subsystem (raids on your own ground) promoted to the marquee, sharing its entire tactical core with SWORN BROTHERS — the two could be one engine, two games.

**Fab.** Medieval fortifications, siege props, and winter sets are deep categories.

**Scope: S–M.**

---

## Comparison table

| # | Idea | Genre | The antagonist | Fab coverage | Scope | Biggest risk |
|---|------|-------|----------------|--------------|-------|--------------|
| 1 | Dirtbag | Climbing life-sim | The grade, the rent | Good; anim gap | L | Climbing hand IK |
| 2 | Three Winters | Saga RPG | The sentence | Deep | M–L | Open-world content volume |
| 3 | Whiteout | Survival | The cold | Good | M | Feel of encumbered movement |
| 4 | The Long Trail | Shooter RPG | The distance | Deep | L–M | Content volume |
| 5 | The Wall | Climbing sim | The wall | Good; anim gap | S–M | Same tech as #1, smaller |
| 6 | Setter's Gym | Tycoon/creative | The members | Good | M | Crowd sim |
| 7 | The Hollow | Narrative horror | The past | Good; anim gap | S | Small audience |
| 8 | Fell | Movement/sports | The vertical | Easy | S–M | Animation feel is everything |
| 9 | Whale Road | Sailing/trading | The open water | Good | M | Sailing feel |
| 10 | Barrow | Dungeon crawler | The dead | Deep | S–M | Combat feel |
| 11 | Sworn Brothers | Tactics RPG | The other wall | Deep | M | Tactics-market crowding |
| 12 | Bloodline | Generational sim | Time | Deep | M–L | Systemic depth vs. scope |
| 13 | Freetrapper | Survival RPG | The winter, the market | Deep | M–L | Cultural care w/ native relations |
| 14 | The Crossing | Survival sim | The miles | Thinner | M | Wagon-era assets |
| 15 | Hotshot | Crew sim | The fire | Good | M–L | Fire-spread tech |
| 16 | The District | Life-sim | Entropy, budget cuts | Deep | M | "Quiet as content" is hard |
| 17 | Gee & Haw | Racing/kennel sim | The trail | **Thin (dogs)** | M | Dog assets & anim |
| 18 | LVDVS 3D | Management | The arena | Deep | M–L | Fight choreography |
| 19 | Basecamp | Management | The window | Easy | S–M | Mostly-UI appeal |
| 20 | The Siege Of | Tactics rogue-lite | The horde, the winter | Deep | S–M | Run variety |

---

## Shortlist & recommended path

**Flagship: #1 Dirtbag.** It's the game the studio clearly wants to make, the 2D version is a complete, play-tested design document, and nothing else on this list has its combination of proven systems and unclaimed genre space. Its one real unknown is climbing tech in Unreal.

**So: pathfinder first — #7 The Hollow or #5 The Wall.** Both are small, genuinely shippable games in their own right, and both de-risk *exactly* the tech the flagship needs: hand-to-hold IK, rock assets, rope systems, conditions and light. This is the landnam-ue method — prove the hard part small before betting the project on it. The Hollow is the smaller, stranger, faster ship; The Wall is the more commercial one and reuses more of Dirtbag's expedition design. Either one, built *as* the first Unreal-Game-2 project and explicitly labeled Dirtbag 3D's pathfinder, is the recommended next step.

**Strongest non-climbing bet: #11 Sworn Brothers.** The shield-wall tactical core already exists in measured, tested TypeScript; turn-based needs the least animation fidelity of anything here; Viking assets are among Fab's deepest; and it shares an engine with #20 if a second game ever wants it.

*Dark horses worth keeping warm: #19 Basecamp (smallest honest build on the list), #10 Barrow (episodic, deep asset coverage, saga-social curses), and #13 Freetrapper (the most commercially shaped survival-RPG premise here).*

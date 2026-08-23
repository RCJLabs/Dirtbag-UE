# The 2D audit — what v0.957 actually has

**2026-08-23.** Evan: *"Wait, so everything other than the actual built world
is finished?? How's that possible? The dirtbag original game had a ton of
features and systems."*

He was right and I was wrong. Here is the receipt.

## What went wrong with the last audit

`notes/what-the-2d-game-still-has.md` says *"Fifteen of the nineteen are
built and measured."* That number is real but it answers the wrong question.
It checked **`concepts/DIRTBAG.md` §4's port list** against `Sim/` — a
nineteen-item paragraph written from memory — and found four gaps.

§4 is not the 2D game. It is a summary of the 2D game, and it is short by
about two orders of magnitude. Auditing against a summary and reporting the
result as *"what the 2D game still has"* is the same mistake as testing an
ordering when the question was a magnitude: the check passes and tells you
nothing. The file has been corrected to say so.

## The method this time

Evan supplied two zips. The deployed one (`dirtbagmain`) is a 12.5 MB
`index.html` with the whole game minified onto line 79 — readable only by
regex. The second (`dirtbag_v0957`) is **the source project**, and that is
the one that matters:

| | lines |
|---|---|
| `proj/src/App.tsx` | **44,171** |
| `proj/src/world.ts` | 1,844 |
| everything else (editor, worldview, recipes) | 2,432 |
| **2D total** | **48,447** |
| | |
| this repo, `Sim/` | 9,709 |
| this repo, `Sim/tests` | 7,695 |
| this repo, `DirtbagUE/Source` | 12,403 |

Two hard structures in that source make the audit checkable rather than
impressionistic:

1. **`GameState`** (App.tsx:95–605) — **497 persisted fields**, most with a
   comment naming the feature that owns them.
2. **A feature-tag taxonomy** — `CLB-16`, `MED-6`, `SETTER-B`, `WRLD-27` and
   so on. **122 families, 2,122 references, ~450 distinct numbered
   features.**

So the check is: take each field and each tag family, strip comments out of
`Sim/`, and look for the concept in the *code*.

Stripping comments matters. A naive substring pass over `Sim/` scored 31
systems present. Twenty-six of those were false — `quest` matched *"the day
becomes a real question"*, `rival` matched *"arrival"*, `trad` matched
*"trade"*, `achievement` matched *"days is not an achievement, it is a
fortnight"*, `olympic` matched the one line of `DirtbagZones.h` that quotes
Evan's own message. Prose about a mechanic is not the mechanic.

## The result

Of the ~88 systems I checked by name against comment-stripped code:

**Present and real — 30.** The five trained skills, projecting/beta,
weather + seasons, partners and bonds, crew naming, the dog, fuel, the odd-jobs
board, the salaried-job trap, factions, sponsorship, injury + physio, aging,
legacy/generations, ethics arcs, dreams, the campfire game, first ascents,
sport + the runout, zones/travel, membership, skin, psyche, crash pads,
pitches, disciplines, route types, gear wear, van parts, acute/chronic load.

**Absent — 50, verified.** Not "thin", not "partial": no code.

| bucket | what the 2D game has that this does not |
|---|---|
| **Competition** | the circuit (season, schedule, points, field, titles), leagues, national ranking points, the **national team** (roster, coach, cuts, seasons), the **World Cup** (schedule, starts, finals, podiums, titles, rounds you skip), the **Olympics** (village, disciplines, medals, appearances), speed PB, dyno comp |
| **Coaching & mentoring** | a mentee with milestone beats, a persistent **coaching roster** with real training plans, the youth team, head-coach days, **gym ownership** |
| **Rivals** | a born rival with a grade that tracks yours, head-to-head standing, alliance, past rivals across generations, rival FAs |
| **Character** | origin, archetype, flaw, personality (4 axes), talents, hidden talents, quirks (picked + surfaced), habits, race, signatures, scars, marks |
| **Medical** | insurance and plans, diagnosis tiers, cortisone (with permanent cost), surgery + aftercare, sickness, meds, **dental** (a staged clock that only escalates), undertreated care, prehab, the shrink, the staged comeback arc |
| **Work as craft** | per-job craft skills (setter, coach, clerk, office, guide, rescue, courier, warehouse, barista, bench), employer standing, work identity, courier accounts, gym sets that stay up |
| **Media economy** | camera, portfolio, print sales, commissions, filmers, clips, followers, posts, the scene's **discourse**, ad campaigns, the **documentary** |
| **Hobbies** | music (busking → rep → paid gigs), reading, the garden at your folks' farm, fishing with a per-species log, cooking + pantry, crafting + scavenged materials |
| **Life** | romance (spark → strain → breakup → cooldown), family, calling home, the farm, achievements/feats, the life list, challenges (daily + weekly), quests with givers and stages, clubs, tax |
| **Van & road** | van build-outs and upgrades, spare parts, propane, water, bivy spots with cooldowns, hitchhikers, the caravan, grime |
| **Places & people** | shopkeepers with memory, cafe regulars, setter regulars, gym regulars, locals, wandering NPCs with eight offer kinds, onboarding/tutorial markers |
| **Climbing** | trad, deep water solo, soloing, expeditions/big wall, guidebook publishing, grade votes and consensus, stints, consumables, the access fund |

Plus five whole minigames the port has one of: cribbage, poker (an N-seat
engine with per-NPC profiles and table talk), blackjack, liar's dice, trivia,
horseshoes. `Sim/DirtbagCampfire` has liar's dice and blackjack.

## The world, specifically

Evan's zones question has a precise answer in `world.ts`, and it is bigger
than what I built from his description:

**Twelve walkable zones in one connected graph** — Notown, Market Row,
Uptown, Grand Plaza, Old Town, Downtown, Midtown, The Lot, Trailhead,
Outskirts, Greenwood Park, Trout Lake. Not a 2×3 town: a 3-wide, 4-deep
city with a graveyard hill at the top and a lake at the bottom, joined by
road arms and driven cars.

**Four walk-in interiors** — the coffee shop, the diner, the cabin, an
indoor mall with eight stations.

**Three van-only destinations** — the crag, the Olympic Village, the folks'
farm.

**Fifty-seven buildings and interactables** across them.

`Sim/DirtbagZones.h` has **five**: Lot, Town, Roadside, Cave, Terrace. The
two-tier model (walk vs van) is right — it is the 2D game's actual rule. The
map under it is a sixth the size.

## What this actually means

Three things, and only the third is bad news.

**The port is not behind on the systems it has.** Thirty systems, measured
rather than assumed, with a 7,695-line harness, eleven preflight checkers and
frozen golden vectors. The 2D game has no equivalent — 44,171 lines with the
rules living inside a React component. Depth per system is genuinely ahead:
the runout model, morphology, hold types, the pump economy, the session
timeline, first ascents, the ethics cascade.

**The scope was never nineteen systems.** It is about ninety, and roughly
450 numbered features. `concepts/DIRTBAG.md` §4 is a sketch, and the roadmap
has been sized against the sketch. That is the thing to fix — not by building
fifty systems, but by making the plan tell the truth about what is left.

**"Everything but the world is finished" was wrong** and I should not have
said it. What is true: *every system the roadmap currently names* is built
and wired. The roadmap names a sixth of the game.

## Recommendation

Do not port fifty systems. Most of them are texture that earns its keep only
once there is a world to put it in, and the ones worth having are worth
picking deliberately rather than by inventory.

What I would do instead, in order:

1. ~~**Re-scope the roadmap against this file, not §4.**~~ **DONE
   2026-08-23.** ROADMAP.md opens with a **scope ledger** — 30 present, 50
   absent, 15 of those recorded cuts and **35 gaps nothing had recorded** —
   and the plan now runs to **Phase 12** instead of stopping at 6: *Who Your
   Climber Is*, *Somebody To Beat*, *The Body Keeps Score*, *A Life Outside
   It*, *Work Is A Craft*, *Trad*. Each has a Done-when gate in house style,
   and 7–11 are marked re-orderable because none of them blocks another —
   all five are engine-free sim work. The Olympics tension is flagged in the
   ledger and in the cut list rather than resolved: §4 cuts comps, the 2D
   zone graph has an Olympic Village, and Evan named it unprompted.
2. **Widen the zone graph now, while it is still five enum values.** Going
   from 5 to 12 zones is an afternoon in `Sim/DirtbagZones.cpp` today and a
   level-rebuild later. The topology is data; the art is not.
3. **Then build the place** (Phase 6's real work), because until there is
   somewhere to stand, the fifty missing systems have nowhere to happen.
4. **Then pick from the list above, in tiers**, with the same rule that has
   worked so far: sim first, measured, one door per system.

The tier-one candidates, if I had to name them: **rivals** (the port has
partners and factions but nobody to beat), **character creation**
(origin/archetype/flaw/personality — it front-loads identity and everything
else reads off it), and **the comp ladder** (the 2D game's whole
mid-to-late-game arc, and the reason the Olympics exist in the zone list).

Everything else can wait for the world.

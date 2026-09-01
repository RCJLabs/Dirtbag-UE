# The 2D audit, third pass — what is actually left

**2026-09-01.** Evan: *"ensure the dirtbag original is neatly ported in the
ways we can do."*

`notes/the-2d-audit.md` is from 2026-08-23 and its numbers are a week and
about fifteen systems stale. It says thirty present and fifty absent. That
was true when it was written and is badly wrong now — since then the port
has taken rivals, character creation, the whole comp ladder, the national
team, the World Cup, the Games, the medical arc, trad, work-as-craft, a life
outside it, gym ownership `GYM-1` through `GYM-12`, the youth team, the gym
league, bivy spots and living-in-the-van.

So: same method, re-run today.

## Method

The 2D source (`App.tsx`, 44,171 lines) carries a feature-tag taxonomy —
`CLB-16`, `MED-6`, `SPEED-3`. Extracted today: **127 families, 2,184
references, 374 distinct numbered features**, each with the comment that
defines it at first use.

Every one of those 374 was checked against **comment-stripped `Sim/`**.
Stripping comments is not optional and the last audit says why: a naive
substring pass scores prose about a mechanic as the mechanic. This pass
caught the same class of false positive again — `tax` matched a route called
*Dyno Tax*, `club` matched a line about a kids club in the gym lobby, `path`
matched *the path menders*, `quest` matched *"the ground stops being the
question now"*, `festival` matched a flyering gig, `family` matched a
trust-fund perk. All six read as present and none of them is.

It also caught two the other way, which is the more interesting direction:
**busking and poker both read as absent by tag and are fully built**
(`BuskingPay`, `DealPoker`/`PlayPoker`). A tag is not a name.

## What is built

**Verified present in code:** the climbing loop, session, pump, skin, nerve,
head and morphology · projecting, beta, the nemesis · conditions, the send
window, forecast, seasons · the crag, the guidebook, first ascents · partners,
crew, bonds, rifts, the crew name · the four factions · the dog · gear,
brands, wear, resoles · kit, shoes, pads · van parts, breakdowns, fuel · the
jobs board, the salaried trap, ten craft skills, employer standing · dreams ·
membership · sponsorship and the deal stack · the ethics cascade · injury,
physio, aging, legacy, generations · the medical arc including insurance,
cortisone, the shrink and the tooth · habits and quirks · archetype, origin,
flaw, temperament, talents · the rival · the comp circuit, leagues, national
ranking, the national team, the World Cup, the Games · gym ownership in full,
the youth team, the gym league · bivy spots · grime, water, propane, cooking ·
locals · trad · the campfire with poker, blackjack and liar's dice · busking ·
zones, travel, the town · the narrator · saves and forty-five migrations.

That is comfortably more than half the tagged surface, and most of it is
deeper here than in the source.

## What is verified absent, and can be built from here

Nothing in this list needs the editor. All of it is `Sim/`.

### Tier one — holes in systems that are otherwise finished

| | what the source has | why it is first |
|---|---|---|
| **`SPEED-1..4`, `OLY-4`, `ZONE-7`** | speed climbing as the third Olympic discipline; per-discipline national ranking (`discRank`), and `natlPts` is the best of the three | **The Games are built and a third of the format is missing.** `Discipline` here is `{Boulder, Sport, Trad}` — the source's comp discipline is `{boulder, sport, speed}`, a different axis. Fully specified in the source, down to the constants. |
| **`TAX-1`** | 20% of the year's prize money, settled once a year, and you can come up short | The port now has comps, leagues, hosting fees, a stipend and Olympic prize money and **taxes none of it**. Small, and it bites everything built in the last fortnight. |
| **`ROSTER-1`, `COACH-5`, `JOB-10`** | a persistent coaching roster — standing clients with real training plans, resolved at league night, and at max craft one of them is a prodigy | `Craft::Coach` exists and the youth team exists; the clients do not. |
| **`WRLD-10`, `TUT-6`** | the mentor arc — Hazel in the lot, before she is your mentor | The port has a rival, partners and locals and **nobody who taught you anything**. |
| **`CHAR-7`, `PSY-2`, `DEPTH-6`** | per-skill staleness, same-place staleness, and `styleXP` — an emergent affinity and anti-style from what you actually climb | Training has no plateau from repetition, and a career of crimping does not make you a crimper. |
| **`DEPTH-8`** | grade consensus — enough goes on a sandbagged line and you know its true grade | The port hides true grades and never lets you learn one except by sending. |
| **`DEPTH-19`** | lifestyle creep — cost of living rises with the grade you climb | The economy's late game has no upward pressure. |
| **`CHAR-6/6b`** | your named signature move, and a second one later | `signature` in `Sim/` is the *rival's* style, not yours. |
| **`CLB-32`** | scouting the rival before a scheduled comp | A direct odds bonus, and the only pre-comp verb. |

### Tier two — new surface, self-contained, no dependencies

- **Hobbies**: fishing with a per-species log and seasonal windows (`FISH-2/3`), the garden at the folks' farm (`GARDEN-1..3`, `FARM-2`), music rep and paid gigs on top of the busking that exists (`MUSIC-2`), reading and the story you tell at the fire (`READ-2`, `HOBBY-1/2`), slacklining (`LIFE-17`).
- **People**: romance from spark to strain to breakup and cooldown (`WRLD-18`), calling home (`FAMILY-1`), partner relationship arcs (`WRLD-17`), the caravan (`WRLD-27`), hitchhikers (`HITCH-1/2`), shopkeeper and cafe memory (`WRLD-28`, `CAFE-2`, `NPC-3`).
- **The bench**: things you build rather than buy (`BENCH-1/2`, `QUAL-1`), the fire you built (`CRAFT-2`), the tarp (`CRAFT-3`), van build-outs and modules (`VAN-1`, `ECON-16`), spares fitted at the roadside (`ECON-11`), field repair on a delaminating shoe (`GEAR-5`, `REPAIR-1`), consumables and chalk stock (`STORE-1`, `ECON-14`), the brand set bonus (`SET-1`).
- **The long tail of a career**: quests with givers and stages (`LIFE-14`), lifetime milestones that grant real perks (`LIFE-16`), feats (`FEAT-2`), epics survived (`EPIC-1`), the close call (`CLOSECALL-1`), the perfect day (`PERFECT-1`), one-time story beats (`DEPTH-15`), legacy boons across generations (`META-2`), the homecoming (`LEGACY-3`).
- **The road**: stints away (`STINT-1`), roadside attractions and a favourite haunt (`LIFE-8`, `FEST-3`, `MINI-3`), the festival and its dyno comp (`LIFE-9`, `FEST-2`).
- **The crag**: steward missions (`WRLD-16`), guidebook pages you write (`WRLD-26`), seasonal closures (`WRLD-9`), local history (`WRLD-25`), the access fund and clubs (`CLUB-1/2`), free soloing (`CLB-22/33/35`), the approach as its own beat (`CLB-30`).
- **The body's last mile**: the med kit (`MED-4`), undertreated care (`MED-5`), daily aftercare (`MED-6/7`), per-region care history (`MED-9`), the tissue-specific comeback (`REHAB-2`).
- **Onboarding** (`TUT-1..6`) — starter tasks, feature intros, meeting the rival.

### Not gaps

Recorded cuts, unchanged: the media economy (`PHOTO`, `FEED`, `DOC`, `AUD`,
`WRLD-8` followers), expeditions and big wall (`ECON-18`, `CLB-16`), deep
water solo, Solo mode (`CLB-34`). Plus about twenty families that are UI and
belong to the editor or to nothing — `QOL`, `MENU`, `FACE`, `A11Y`, `UX`,
`NAV`, `DIARY`, `LOG`, `WHATS`, `LAUNCH`, `FEEL`, `TRAFFIC`, `EDITOR`,
`IMMERSION-1`, `SAVE-4`.

## The recommendation, and it has not changed

Do not build the tier-two list as an inventory. Build tier one, because
**every item on it is a hole in something already finished** — and a system
that is 90% ported reads as a bug, where a system that is 0% ported reads as
a plan.

Tier two is texture, and `notes/the-2d-audit.md` was right that most of it
earns its keep only once there is a world to put it in. Phase 6 is still the
milestone and still cannot be done from here.

**Start with speed.** It is the largest single hole, it is fully specified in
the source down to the constants, it is the only thing standing between the
Games and the actual Olympic format, and it is a player-driven beat — a
reaction start and a rhythm up sixteen rungs — which makes it the first thing
in the port since HOLD TO CLIMB that is a *minigame* rather than a ledger.

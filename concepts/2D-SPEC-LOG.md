# 2D spec log

The 2D game is this project's spec (see `CLAUDE.md`). When the spec changes, the change is logged here with what it means for the Unreal version, so this repo never follows a spec it hasn't read. Newest first.

---


## 2026-10-02 — Phase 23.8: when calls and echoes come

**Changed in the 2D rebuild's Phase 23.8** (amends 23.5). No save change.

- **When:** checked after a `travel` or a `done` (the end of a go), not only on arriving by road, unless an encounter is already up, the van's broken down, you're on an expedition, or dead. The roll is seeded by day and crag, so repeated checks on one crag-day don't add chances.
- **Where:** an echo can come anywhere (several are set off the rock: a gym car park, an access meeting); a new call only at a crag.
- **Naming a first ascent** (`{ t: 'name' }`) is allowed while an encounter waits, since it finishes the go that came before it. The client keeps a go's own card (fall, send, first ascent, the send card) up until it's closed, then shows the encounter.
- **Measured:** bots answering each call the way its echo comes back for see every echo land within a 224-day career (most on the first seed; the latest at day 155).

## 2026-10-02 — Phase 23.7: the year and the Homecoming

**Built in the 2D rebuild's Phase 23.7.** Save v37: `year` (`recapped`: years recapped; `home`: `'none'`, `'armed'`, or the day it was had). Action `{ t: 'home', arm }`; events `{ k: 'year', n }`, `{ k: 'homecoming', route, who }`.

- **A year is 56 days** (`YEAR.days`). After every action, when `floor((day − 1) / 56)` exceeds `recapped`, it's recapped: a card for that year, built from state by day ranges (first sends excluding expedition pitches, the hardest of them, crags sent at, your own first ascents, Record Book entries, expeditions home, calls answered, cash in hand). Migration marks years already done as recapped, with no card.
- **The Homecoming,** once a life: available when at least `HOME.people` (2) people are at bond tier ≥ `HOME.tier` (2, Regular), counting people (v0.956 counted diary entries). Armed (or put off) from the You page; on the next action that sends a line outside (not a repeat, not an expedition pitch) while armed, it pays `HOME.cash` (200) and `HOME.psyche` (20), records the day, and names who came.

## 2026-10-02 — Phase 23.6: the Board

**Built in the 2D rebuild's Phase 23.6.** Save v36: `board` (`week`, `jobs`: `{id, grade, paid}`, `shifts0`: shifts worked when it went up, `sessions`: sessions since).

- **One weekly board** replaces v0.956's café board, daily and weekly challenges and club jobs (`content/board.ts`, 14 jobs, de-duplicated). After every action, if `weekOf(day)` differs from `board.week`, a new board is posted: `BOARD.jobs` (3) jobs, a seeded shuffle on `derive('board-{week}')`, each stamped with `gradeOf(skills)` at posting.
- **Criteria:** sends anywhere; at ≥ grade + `over`; outside; outside at ≥ grade + `over`; of a route type; training sessions (counted in `train`); shifts (sum of `jobs` minus `shifts0`). Sends count first sends with `sent.day` in the board's week; expedition pitches don't.
- **Pay:** automatic, once, the moment progress reaches `n`, after every action: `cash` (8–18) and a line. Unfinished jobs lapse with the week; no penalty.
- **Harness:** the three best-paying jobs together pay under `BOARD.capDays` (2) days of the worst one-shift job's pay.

## 2026-10-02 — Phase 23.5: the scene, factions and stances

**Built in the 2D rebuild's Phase 23.5.** Save v35: `scene` (`old`, `gym`: standing 0..100 from 50; `stances`: `{id, opt, day}`; `echoes`: `{id, turned, day}`; `last`, `echoLast`: days of the last call and echo). Encounter kinds gain `stance` and `echo`.

- **Two factions:** the old guard (v0.956's trad, and the locals) and the gym crowd (its gym and comp). v0.956's media, purism, rep, followers and access fund are dropped. Words at 25 / 45 / 56 / 76: Distrusted, Skeptical, Neutral, Respected, Beloved.
- **What moves them:** answers to calls and echoes only, plus taking up a calling (Purist old +15; Send-or-Bust gym +13; Lifer old +8, gym +5). Sends don't (v0.956's per-send drift would saturate both).
- **Calls** (`content/scene.ts`, v0.956's five): on arriving at a crag (not a gym), with no road encounter, if at least `FACTION.gap` (14) days since the last call, a seeded `FACTION.chance` (0.22) picks one not yet faced whose `after` day has come (chip, closure, trashed 14; retrobolt, fa 28; fa also needs a first ascent of your own). Three answers; a fourth (the elder's) when `scene.old ≥ FACTION.elder` (65). Each answer: standing deltas, psyche, energy, and a journal line.
- **Echoes** (six): on arrival, before a call, the first echo whose stance and answer match a call made at least `FACTION.echoAfter` (30) days ago, if `FACTION.echoGap` (25) days since the last echo. Two answers: hold or turn.
- **Perks of standing** (v0.956's clubs folded in), as edges: old guard 56: crux windows outdoors ×1.02; 76: another ×1.02, and permits cost nothing. Gym crowd 56: session gains ×1.15; 76: another ×1.13.

## 2026-10-02 — Phase 23.4: paths, mastery and a quirk

**Built in the 2D rebuild's Phase 23.4.** Save v34: `paths` (`tiers`: path id → tiers claimed; `told`: "id:tier" already announced), `mastery` (styles mastered), `quirk` (id or null), `habits` (counts of goes: `goes`, `outdoor`, `fresh`, `tired`, `evening`, `dawn`, `easy`, `power`).

- **One edge type** (`content/paths.ts`) feeds every source: `windows` (crux windows ×, on lines of `style` if given), `firstGo` (× on a line never tried and never sent), `gym` (× indoors), `fresh` / `tired` (× at energy ≥ 70 / < 35), `evening` / `dawn` (× from 17:00 / before 09:00), `injury` ×, `gain` (one skill's learning ×), `gainAll` ×, `gas` ×, `living` ×. Edges multiply. v0.956's "+n% odds" became windows × (1 + n).
- **Paths:** Crusher (power, dyno), Tendon (crimp), Nerve (first go), Mover (technical; new), Dirtbag (trips); Scene waits for comps. Three tiers each; tier i needs `gradeOf(skills) ≥ PATH.gates[i]` (4, 9, 14) and its deed (sends in its styles 4 / 12 / 25 for the style paths, with Tendon's 2nd at 12 crimps and Crusher's 2nd 6 dynos; Nerve 3 / 8 / 15 flashes or onsights; Dirtbag 6 / 25 / 60 trips). Claimed by action `{ t: 'path', id }`, a tier at a time, at most `PATH.max` (2) paths. Each newly claimable tier is announced once. Tier 3 gives a title and line, no active ability.
- **Mastery,** one track, automatic, checked after every action: a style at `MASTERY_AT.sends` (40) lines sent in it (v0.956's style-mastery edges), and v0.956's ten hybrids, each two skills both at `needFor(MASTERY_AT.hybrid)` (V8's) with v0.956's hybrid edges. Ids share the `mastery` list (`hy_*` for hybrids).
- **Quirk:** each go counts into `habits` before its costs. Once `goes ≥ 30` and no quirk, the first of v0.956's tests that passes names it (Glass Cannon, Stone Purist, Weekend Warrior, Grinder, Night Owl, Sandbagger, Slow Burn at day 150; Comp Beast waits). One quirk, not v0.956's two.
- **Migration:** paths and quirk empty, habits zero, mastery backfilled quietly from sends.

## 2026-10-02 — Phase 23.3: callings

**Built in the 2D rebuild's Phase 23.3.** Save v33: `calling` (`id` or null, `since`: the day taken up, `rungs`: the day each was met, `offered`).

- **The offer:** after any action, a climber with no calling, never offered, and at least `CALLING.offerAt` (10) lines sent is offered once (event `{ k: 'calling' }`, `offered` set). Taking one up (`{ t: 'calling', id }`) is permanent; only the open callings can be taken.
- **Callings** (`content/callings.ts`), fx [proposed]:
  - Purist: `gainOut` 1.15 (skill from goes outside ×).
  - Send-or-Bust: `firstGo` 1.05 (crux windows × on a line never tried and never sent; shown on the beta bars), `injury` 1.1.
  - Lifer: `living` 0.8 (the night's spot and lifestyle ×), `injury` 0.75 (on any line, unlike tendon talents).
  - Influencer: no fx, `wait`: not offered until followers and sponsors exist.
- **Rungs,** counting only sends whose `sent.day >= since`: Purist, a line sent outside at V8 / V11 / V14; Send-or-Bust, flashed or onsighted outside at V6 / V9 / V12; Lifer, 5 / 9 / 13 grades each with `CALLING.consolidate` (5) lines sent. Checked after every action, in order (a rung only after the one below); each met adds the day to `rungs`, psyche `CALLING.psyche[i]` (8, 12, 16) and a note.
- v0.956's personality seeds and faction shifts from callings are cut.

## 2026-10-02 — Phase 23.2: origins and talents

**Built in the 2D rebuild's Phase 23.2.** Save v32: `origin` (an id, `'across'` for a v0.956 climber, or null) and `talents` (`ids`, `known`, `from`: the skills at creation).

- **Six origins** (`content/origins.ts`), picked at creation with a name and a start. Each adds `skills` to the start's (floored at 1) and has `fx`, all [proposed]: `pay` (shift pay ×, after rank raises), `gainIn` / `gainOut` (skill from goes indoors / outside ×), `gainTrain` (from sessions, the speed wall's too ×), `spring` (power and fingers from anything ×), `living` (the night's spot and lifestyle ×), `premium` (insurance ×), `shop` (kit and food bought ×), `bill` (dollars a week on the weekly bills), `cash` (on top of the start). Prices round after the multiplier, and the UI shows the same functions' results.
  - Sold It All: pay 1.12, +$150, $8/week; endurance +2, head +2.
  - Gym Rat: indoors 1.12, outside 0.9; power +3, fingers +3, head −3, endurance −1.
  - Desert Local: living 0.85, −$25; head +4, technique +3, power −3.
  - Ex-Gymnast: sessions 1.2; power +5, fingers +2, head −5, technique −2.
  - Late Bloomer: premium 0.65, +$120, spring 0.88; head +4, technique +3, power −3, fingers −2.
  - Trust-Fund Kid: shop 0.7, +$250, pay 0.85; technique +2.
- **Talents** (`content/talents.ts`, v0.956's 11): with an origin, one good and one bad are dealt on `Rng.fromStream(seed,'events').derive('talents')` (good list index, then bad). Each multiplies all gains of one skill (1.35 / 1.1 good, 0.7 bad); Bomber and Glass Tendons also multiply injury chance on crimp and crack lines (0.55 / 1.7). Hidden until that skill is `TALENT.reveal` (6) above `from`, or, for a tendon talent, on a finger injury; then a note and `known`.
- **A carried v0.956 climber** gets `origin: 'across'`, no fx, no talents. No origin (old saves, tests, bots) means no fx and no talents.
- **Act I's first goal** ($60 in hand) is met at creation for the richer origins, as in v0.956.

## 2026-10-02 — Phase 23.1: the Record Book

**Built in the 2D rebuild's Phase 23.1.** Save v31: `record` (entry id → the day earned).

- **One book** replaces v0.956's feats, milestones and story cards: 37 entries, each carrying the v0.956 ids it merged (`from`). 27 have an `aim` and are earnable now; 10 `wait` for systems not yet built (comps, media, sponsors, guidebooks, giving, retirement, traits) and are hidden until then.
- **Earning:** after every action (once a climber exists), every entry not yet in the book whose aim is true goes in, dated today. Aims read existing state only: sends (count, highest route grade, flash/onsight, outside), first ascents, every open crag sent, grade above Dex's, injury or scar, day > 56, a shift worked (walk-ins included), a job rank ≥ 1, ever let go, cash ≥ N, kit kinds ≥ N, a dream owned, a breakdown, a bond tier ≥ 3, an expedition summit, a dog.
- **One moment, one card:** all entries earned by one action make one event, `{ k: 'record', ids }`; the card shows the first entry's story and names the rest.
- **No reward** beyond the entry. Ids are stable for Steam achievements (Phase 14).
- **Migration:** an older save's record is filled with every entry its state already meets, dated the load day, with no cards.

## 2026-10-02 — Phase 24.9: a summit pays once

**Built in the 2D rebuild's Phase 24.9**, confirmed by Evan (2 Oct 2026). No save change.

- **Pay:** an objective's `pays` is paid on its first summit only: if the book holds a summit of that objective, a summit pays 0. (Before, every summit paid, and a climber two grades past El Cap netted about +$1,175 a trip.)
- **No-farm target:** for each objective at every grade from `gradeReq` to `grade + 2`, the bots' mean net cash (pay − trip − food; bills excluded) per calendar day away, for a repeat trip, must be under the worst-paid shift's daily pay.
- **UI rule:** while a go is under way on an expedition pitch (and its stamp shows), no sheet covers the wall; the day's card returns after.

## 2026-10-02 — Phase 24.8: Trango's scene

**Built in the 2D rebuild's Phase 24.8.** No rule or save change; what the scene shows is spec.

- **While away on Trango,** the backdrop is base camp on the Trango glacier: the Nameless Tower (golden, square-topped) on its snowy shoulder, Great Trango left, the Monk and the Pulpit right, Karakoram snow peaks behind, the glacier with medial moraines and base camp's tents. Variants: day, night (one tent lit), storm (towers in cloud, snow), storm at night.
- **Your position:** the portaledge marker follows Eternal Flame (approach gully, snow ledge, south face) by `pitch / pitches`.
- **A pitch's wall view** is golden granite with a snowy belay ledge and the glacier below.
- **All three objectives** now have their own scene and wall view; storms snow wherever the objective's water is snow.

## 2026-10-02 — Phase 24.7: Cerro Torre's scene

**Built in the 2D rebuild's Phase 24.7.** No rule or save change; what the scene shows is spec.

- **While away on Cerro Torre,** the backdrop is the view from above Laguna Torre: the needle with its rime mushroom, the Southeast Ridge as its left skyline from the Col of Patience (El Mocho beside it), Torre Egger and Standhardt right, the ice cap behind, the Torre glacier, moraine and lake with bergs below. Variants: day (lenticular clouds), night, storm (summit plume, cloud on the towers), storm at night.
- **Precipitation:** storms on objectives whose water is snow (`melt`) fall as snow, on the scene and the wall view; El Cap's are rain.
- **Your position:** the portaledge marker climbs the Southeast Ridge by `pitch / pitches`.
- **A pitch's wall view** is rimed granite with ice in the corner, a snowy belay ledge, the glacier below; your partner belays from the ledge.

## 2026-10-01 — Phase 24.6: El Capitan's scene

**Built in the 2D rebuild's Phase 24.6.** No rule or save change; what the scene shows is spec.

- **While away on El Cap,** the backdrop is El Cap Meadow: the wall (the Nose between the lit southwest face and the shaded southeast; the Heart; the North America diorite), the forest at its foot, the meadow. Variants: day, night (moon, stars, other parties' headlamps), storm (summit in cloud, rain), storm at night. Weather is the trip's `stormOn` for the day.
- **Your position:** a portaledge marker at `pitch / pitches` of the way up the line, with the count beside it.
- **A pitch's wall view** is El Cap granite with a belay ledge at its foot, the haul bag and portaledge hanging below, and the meadow far down. Your partner belays from the ledge (not whoever is at the Lot).

## 2026-10-01 — Phase 24.5: coming home

**Built in the 2D rebuild's Phase 24.5.** Save v30: `book` (a list of trip logs).

**The rules:**
- **Every end of a trip** (summit, bail, out of water, out of days) runs one path: expedition cleared; bond with the partner moved by +2 (summit), −1 (water), +1 (otherwise, if pitches fixed × 2 ≥ the objective's pitches), else 0; the `home` travel nights; then a log appended: `{id, day (calendar, after travel), end, high (pitches fixed), partner, nights, seen, told: false}`, and a `home` event (the UI's trip card).
- **High point:** per objective, the max `high` over its logs, and whether any ended in a summit.
- **The story** (`story` action): only for the newest log, untold; at the Lot after dark with someone at the fire. Costs 30 min; psyche +8 for a summit, +4 otherwise (clamped to 100); each person at the fire counts it as a day together (the once-a-day bond); marks it told.

## 2026-10-01 — Phase 24.4: what happens up there

**Built in the 2D rebuild's Phase 24.4.** Save v29: `expedition.seen` (ids of this trip's events).

**The rules:**
- **Events as data:** six, each two options with effects `{energy, food (days), pitch, ledge: false, bond, psyche, day}`. `stove` only where the objective's water is snow and a stove was packed; `party` only with a partner.
- **When:** after each portaledge night that doesn't end the trip, if fewer than 2 have come: roll `Rng.fromStream(seed, 'events').derive('wall-{objective}-{calendar day}')`; under 0.15, pick uniformly from the eligible, unseen events with the same stream's next draw. The event is an encounter (`kind: 'wall'`) and blocks everything but its answer.
- **Answer:** apply the effects (energy and psyche clamped to 0–100, food and pitch floored at 0); `day` runs another portaledge night, which can bring the next event.
- **Odds:** the DP carries the count of events so far; on any day after the trip's first, other than today, with fewer than 2, an event comes with p = 0.15 and is modelled as the day lost (no climbing, no night's cost beyond the day).

## 2026-10-01 — Phase 24.3: getting there and back

**Built in the 2D rebuild's Phase 24.3.** No save change.

**The rules:**
- **Travel:** each objective has `out` and `home` days (El Cap 1/1, Cerro Torre 2/2, Trango 8/6). `go` runs `out` away-nights (bills, psyche, the day count) before the wall's day 1; every way a trip ends (summit, bail, out of water, out of time) runs `home` away-nights back to the Lot.
- **Trip window:** from the booked day to day + out + days + home − 1. Sign-ups inside it are refused while booked. Booking ≥ 7 days ahead removes signed-up shifts inside it; otherwise they stay and are missed (a warning each, as any missed shift).
- **Leads per day:** at most 3 goes on expedition pitches a day (counted across the wall's pitches).
- **Odds:** the wall's day 1 is the booked day + out for the forecast. Each pitch's send chance is played at an estimated skin: skin − (trip day − today) × max(0, 3 × the wall's mean skin per go − 22) − 2 × that per-go skin, and the DP's goes a day are capped at 3.

## 2026-10-01 — Phase 24.2: planning and packing

**Built in the 2D rebuild's Phase 24.2** (`dirtbag/app/src/sim/expeditions.ts`; `EXPED` in `dials.ts`). Save v28. Evan confirmed 24.1's partner grades (yours −1 Hazel, +1 Sage) and handing over a pitch.

**The rules:**
- **Plan:** `{ day, partner, food, ledge, stove }`. Book (`exped do: 'book'`) up to 7 days ahead; pays the trip cost (War Chest halves it) + $12 × food. Refused under the grade gate, roped without an eligible partner, solo with one, food outside 1..days−1, or a bag over 80 kg. `booked` holds it; `cancel` refunds half (rounded); a sleep past the day with no expedition drops it, no refund. `go` only on the booked day, at the Lot.
- **Bag:** kg = food × 3 × (melt objective without stove ? 2 : 1) + 9 (portaledge) + 2 (stove). Cerro Torre and Trango are melt objectives.
- **Nights:** a camp needs food > 0 (else: down, trip over); food −1; energy back = max(0, round(nightBack(nights) × (ledge ? 1 : 0.5) − max(0, kg − 35) × 0.4)), kg after eating.
- **Forecast:** for a day `lead` days from today, a call of storm/clear = truth, flipped if a seeded roll (`fc-{id}-{day}-{lead}`) ≥ accuracy, accuracy = 1 at lead ≤ 0, else max(0.5, 1 − lead / 20). Storm chance used by the odds = the Bayes posterior of the call given stormOdds and the accuracy (today: the truth).
- **Odds:** the 24.1 DP, with each day's storm chance from the forecast, and the trip ending when food runs out.

**What it means here:** the expedition card is a planner whose every choice moves the shown summit odds.

## 2026-10-01 — Phase 24.1: expeditions climbed

**Built in the 2D rebuild's Phase 24.1** (`dirtbag/app/src/sim/expeditions.ts`, pitches in `sim/content/expeditions.ts`; `EXPED` in `dials.ts`). Replaces 21.5's dice-per-pitch model. Save v27.

**The rules:**
- **Pitches:** each objective has `pitches` lines (6/8/11), each a single-crux route like a valley wall pitch (`libraryPitch`), graded up to the objective's grade, with `exped: id`. Ids `elcap-1`…
- **State:** `expedition = { id, day, pitch, partner | null, nights }`. While set, only `exped` actions and goes on the current expedition pitch are allowed.
- **Go:** needs the grade gate, cash in hand (War Chest halves), and on a rope a partner: the highest-bond person with a `grade` at bond ≥ BOND.tiers[2]. Free Solo: partner null.
- **Blocks:** pitch i is yours if floor(i / 2) is even (solo: all yours). A go on your current pitch costs 150 min, 15 energy, 6 fed; refused in a storm (seeded `storm-{id}-{day}` < stormOdds), after dark (19:00), or under 10 energy. A send advances the pitch; the last pitch pays and teaches head +2.
- **Follow** (their block, or yours handed over; once a day; not in a storm): the partner tries up to 2 times on consecutive pitches, each fixing it if a seeded roll (`lead-{id}-{day}-{t}`) < clamp((0.4 + 0.1 × (partnerGrade − pitchGrade)) × windows, 0.05, 0.95). Partner grade = your grade + offset (Hazel −1, Sage +1). Costs you 15 energy.
- **Camp:** a portaledge night: energy + round(45 × 0.92^nights), fed raised to 60, day + 1. Past `days`: home, nothing paid.
- **Windows up there:** body factors × thin (El Cap 0.6, Cerro Torre 0.65, Trango 0.65) × max(0.82, 1 − 0.015 × (day − 1)). No valley weather, sun or crowd.
- **Odds:** each of your pitches' send chance per go = the fraction of 24 seeded plays with human-ish hands (bot `humanHands`), at trip days start/middle/end (interpolated), with food as of the day's third go. Then an exact DP over (day, pitch, energy, nights): storm with stormOdds; your block = goes until energy/light; partner's day = the 2-try distribution; on your block, the better of leading and handing over.

**What it means here:** an expedition is a long wall of real climbing, and the odds are a model of the climbing, not a separate roll. Cerro Torre's objective is now the Southeast Ridge.

## 2026-10-01 — Phase 22 closed: Evan's calls

**Rule change (2D 0.984.0):** a night at cards (blackjack or hold'em played today) counts as psyche's "company", once a day, the same as climbing with someone. It moves no bond.

**Confirmed as built** (no longer proposals): the fire games' bond at most +1 per person per day however many games; no win bonuses from v0.956; Sage at the fire 40% of nights at tier ≥ 2; blackjack pays ceil(1.5 × bet) on a blackjack ($8 on $5); no insurance; the hold'em styles; v0.956's history trivia stays out; the dreams keep v0.956's prices.

**Next:** the 2D build's CURRENT MILESTONE is Phase 24, expeditions as trips.

## 2026-10-01 — Phase 22.9c: the glossary

**Built in the 2D rebuild's Phase 22.9c** (`dirtbag/app/src/sim/content/glossary.ts`). Content only, no rules or save change.

**The spec:** v0.956's trivia game is gone. Its technique, gear and culture terms are entries in a glossary, with the rebuild's own climbing words: 45 terms in five groups (on the wall, how it went, kit, grades, the life), each a term and a short definition, shown as a journal page. Nothing unlocks or pays. v0.956's history questions are not carried.

**What it means here:** a reference screen in the journal, reading the same data. The text is the spec; keep it in sync with the 2D content file.

## 2026-10-01 — Phase 22.9b: blackjack and hold'em

**Built in the 2D rebuild's Phase 22.9b** (`dirtbag/app/src/sim/cards.ts`, styles in `sim/content/cards.ts`; `GAMES.bj`, `GAMES.holdem` in `dials.ts`). Evan's calls on stakes and shape. Save v26. Simulated gambling: store content ratings must say so.

**The rules:**
- **State:** `cards = { game: 'bj' | 'holdem', who, chips, hands, bj, he } | null` and `reads: Record<who, { hands, caught }>`. While `cards` is set, only that game's actions are allowed.
- **Sit:** at the Lot at night with someone at the fire, ≥3 energy, cash ≥ one hand's need ($5 blackjack; $11 hold'em, the ante plus every bet); 60 min, 3 energy, once a night per game. Chips = min(floor(cash), $40), taken from cash; leave returns them.
- **Blackjack:** `{ t: 'bj', do: sit | deal | hit | stand | double | leave }`, dealt by the first at the fire. Deck per hand from `bj-{day}-{hand}`, a full 52 shuffled; you get cards 0 and 2, the dealer 1 and 3. Either blackjack settles at once (dealer peek). Hit to 21 or bust settles. Double: two cards only, bet doubled, one card, then stand. Dealer draws below 17 and stands on all 17s. Pays: blackjack bet + ceil(1.5 × bet); win 2×; push 1×. 10 hands a night, $5 a hand.
- **Hold'em:** `{ t: 'holdem', do: sit | deal | fold | call | bet | leave, who }`, heads-up with a chosen person at the fire who has a style. Deck per hand from `he-{who}-{day}-{hand}`: you 0 and 2, them 1 and 3, board 4–8. Ante $1 each. Streets 0 (no board), 1 (flop, 3 cards), 2 (turn and river, all 5); one bet a street of $2/$4/$4. With no bet facing you: check (call), bet or fold. They respond to your bet by calling or folding, and to your check by betting (you then call or fold) or checking. No raises. Showdown after street 2; ties split.
  - **Their play:** equity e = a 60-deal seeded sample of their hand against a random one, given the visible board. Facing a bet: call if e ≥ style.call − 0.15 × theyKnow. Else: bet if e ≥ style.bet, or on a seeded bluff roll < style.bluff. Hazel 0.50 / 0.68 / 0.05; Sage 0.40 / 0.56 / 0.30.
  - **Reads:** every hand played increments `reads[who].hands`. A loss at showdown after you bet street 2 increments `caught`. theyKnow = min(1, caught / hands / 0.25), and 0 below 5 hands. Reads unlock at 5 hands (how they bet) and 12 (bluff rate as "one in N", from style.bluff).
- **Pay:** money only. No bond.

**What it means here:** two money games, kept near even. A sensible player wins about $1 a night at hold'em and blackjack is roughly break-even; the hold'em opponents adapt to a player who bluffs too much.

## 2026-10-01 — Phase 22.9a: horseshoes and liar's dice

**Built in the 2D rebuild's Phase 22.9a** (`dirtbag/app/src/sim/fire.ts`; `GAMES` in `dials.ts`). Two of v0.956's fire games, rebuilt with a time cost and a bond cap. Save v25.

**The rules:**
- **Who's there:** at the Lot after dark. Hazel always; Sage if bond tier ≥ 2, not away, on a seeded 40% of nights (`sage-fire-{day}`).
- **Each game:** 60 min, 3 energy, once a night (`today` gets `shoes` / `dice`). Refused away from the Lot, by day, with nobody there, or under 3 energy.
- **Bond:** each player at the table gets the same call as climbing together: +1 bond and `last = day`, at most once a day per person, however many games. `last = day` is psyche's "company".
- **Horseshoes:** `{ t: 'shoes', throws: [4 × 0 | 0.5 | 1] }`, scored by the player's timing on the busking beat: 1 = ringer (3 pts), 0.5 = leaner (1), 0 = miss. Each other player's 4 throws come from `shoes-{who}-{day}`: ringer 25%, leaner 35%. A line says the score against each.
- **Liar's dice:** `{ t: 'dice', do: 'deal' | 'call' | 'raise' }` against the first at the fire. Deal (from `dice-{who}-{day}`): 5 dice each; their bid is on their most-held face (ties to the higher), at held + 0/1/2 (35/40/25%). State `table = { who, mine, theirs, bid }` blocks every other action until resolved.
  - call: you're right if both cups hold fewer than the bid.
  - raise: your most-held face at the bid's count if that face is higher, else count + 1. They call if it needs more than 1 of your dice past what they hold; otherwise they let it go (yours).
  - Nothing is staked; the result is a line.

**What it means here:** v0.956 paid +1 bond to Sage, Rico and Mara per game with no time and no cap (the audit's bond farm). The rebuild prices a game in an hour and caps bond at a day's worth per person. Liar's dice's opener is tuned so the player's own dice are the tell (calling holding none of the face is right ~64% of the time).

## 2026-09-30 — Phase 22.8: dreams

**Built in the 2D rebuild's Phase 22.8** (`dirtbag/app/src/sim/dreams.ts`, content in `sim/content/dreams.ts`; `DREAM` in `dials.ts`). v0.956's dreams and prices. Save v24.

**The rules:**
- **State:** `dream = { pick, pot, owned[] }`.
- **Actions:** `{ t: 'dream', do: 'pick' | 'stash' | 'take' | 'claim' }`.
  - pick: any dream not owned.
  - stash: an amount from cash in hand, refused if cash < amount (never the card).
  - take: the whole pot back to cash.
  - claim: pot ≥ the picked dream's cost; the pot pays it, the dream is owned, and pick is cleared.
- **The pot:** bills and card spending never touch it. The broke check (the hustle's teach line) uses cash + pot.
- **Dream Rig ($2,800):** every spot's price × 0.5 (rounded; gas unchanged), and every van upgrade set owned.
- **War Chest ($5,000):** expedition cost × 0.5 (rounded).
- **Home Base ($9,000):** the Lot's spot price 0 and Lot ticket odds 0, and a kitchen owned. The weekly bills are unchanged.

**What it means here:** long-horizon savings goals with a protected pot and permanent cost modifiers. At the 2D build's worker saving rate, they take roughly 170, 300 and 550 days (a tuning note, not a rule).

## 2026-09-30 — Phase 22.7: Scout's life

**Built in the 2D rebuild's Phase 22.7** (`dirtbag/app/src/sim/scout.ts`, text in `sim/content/dog.ts`; numbers in `DOG` in `dials.ts`). v0.956's lifecycle, words and numbers. Save v23.

**The rules:**
- **Perks by bond** (and only while fed, ≥30). Each is said the day bond crosses it:
  - settle, ≥20: the fire's psyche lift +2;
  - find, ≥45: each van night, `events`, `scout-find-{day}`, p 0.25, +1 serving of a random ingredient;
  - watch, ≥75: Lot ticket odds × 0.5.
- **Age:** floor((day − since) / 18) dog-years.
- **At the night's turn over** (age at the ended day < n ≤ age at the new day):
  - n = 8: the gray line;
  - n = 12: the senior line;
  - n = 10: The Limp, −$70; n = 13: The Lump, −$130 (flat cash, not insurance).
  - Gray and older: the crag lines are the slower set.
- **The end:** on a van night when age ≥ 16, encounter `farewell` (id = the dog's name). Every action but the answer is refused.
  - Three answers, text only. After one: the closing line (best-friend version if bond ≥ 70) and a log entry "{name}, {years} years. {legacy}. Best dog in the valley and everybody knew it."
  - Then `dogs.push({ name, years, day })` and `dog = null`.
- **After:** the stray offer needs 30 days since the last dog went. Names go Scout, Moss, Juniper, Biscuit, by how many you've lost.

**What it means here:** a companion with bond-gated passive perks, a fixed aging timeline with scripted beats and costs, and a permanent, player-shaped ending.

## 2026-09-30 — Phase 22.6c: walk-outs

**Built in the 2D rebuild's Phase 22.6c** (`dirtbag/app/src/sim/events.ts`, content in `sim/content/epics.ts`; numbers in `EVENTS.epic` in `dials.ts`). v0.956's text and numbers; the headlamp's price is [proposed]. Save v22.

**The rules:**
- **Trigger:** a `travel` from a crag, checked before the drive. Needs: day ≥ 3, not injured, no encounter today, ≥10 days since the last walk-out.
- **Risk:**
  - Late: +2 if min ≥ 19:00, +1 more if ≥ 21:00, and +3 more if late without a headlamp.
  - Conditions: +2 for rain at the crag, +1 in winter.
  - The body: +2 if energy < 25, else +1 if < 45; +1 if food ≤ 15; +1 if supplies ≤ 0.
  - +1 if you climbed with nobody today.
- **Odds:** clamp((risk − 4) × 0.055, 0, 0.45) against `events`, `epic-{day}`.
- **Kind:** rain → storm; on a wall → stuck; late in winter → cold; late → dark; else lost.
- **State while it runs:** `encounter = { kind: 'epic', id: kind, stage, tally }`. Each answer adds its option's risk, energy, fed, skin, psyche and hours (1 when unspecified); three stages.
- **After the last stage:** the time passes (hours × 60 min) and energy, fed and skin change by the tally; psyche changes by (tally + 5) × 0.5. The end is clean if risk ≤ 2, rough if ≤ 6, else bad.
  - A bad end is an injury: 50/50 tier 1 or 2 ("jammed" or "rolled ankle"), days from the injury table, billed as any injury.
  - The ending's line is said, and the story goes in the log.
- **Afterwards:** you're still at the crag and drive normally.
- **The headlamp:** owned gear, $18 at the gear shop.

**What it means here:** a multi-stage choice sequence gated by a risk score from the day's state, resolving by thresholds into outcomes with a persistent story line.

## 2026-09-30 — Phase 22.6b: hitchhikers and roadside stops

**Built in the 2D rebuild's Phase 22.6b** (`dirtbag/app/src/sim/events.ts`, content in `sim/content/road.ts`; numbers in `EVENTS` in `dials.ts`). v0.956's text and hitchhiker numbers; stop odds and the day-3 start [proposed]. Save v21.

**The rules:**
- **No encounter of any kind before day 3** (this now applies to knocks too).
- **Trigger:** a `travel` to or from a crag that doesn't break down, with no encounter yet today. Roll: `events`, `drive-{day}-{min}` (min on arrival).
  - First draw < 0.14, and ≥5 days since the last hitchhiker: a hitchhiker, picked by weight (2.6 if met, else 1) with a second draw.
  - Else, if the first draw is in [0.14, 0.34) and ≥3 days since the last stop: a stop, drawn from the unseen ones (all, once every one is seen).
- **The encounter is set after arrival;** every action but `answer` is refused until it's answered.
- **Hitchhiker, first meeting:** their 3 answers plus "drive past" (index 3: no effect, not remembered). Picking an answer records `met[id] = index`.
- **Hitchhiker, later meetings:** their 2 `again` answers, minus any whose `when` differs from `met[id]`.
- **Effects:** psyche × 0.5; cash; energy; `beta`, the most useful unknown beta at the destination (the one a watching partner would teach); `guitar` (+sets).
- **Stop:** 0 = pull over (40 min; effects × 1 the first time, × 0.4 after, psyche then × 0.5), 1 = keep driving.
- **Breakdown:** with no friend to call, a met hitchhiker other than the grifter or thief comes along on a seeded 50% (`events`, `friend-{day}`), and works as the friend fix.

**What it means here:** a travel-triggered encounter layer with persistent memory of each NPC's first choice, feeding a later rescue.

## 2026-09-30 — Phase 22.6a: the event deck, and knocks at night

**Built in the 2D rebuild's Phase 22.6a** (`dirtbag/app/src/sim/events.ts`, content in `sim/content/knocks.ts`; numbers in `EVENTS` in `dials.ts`). v0.956's numbers and text. Save v20.

**The rules:**
- **State:** `deck = { last, knock, seen[] }` (day of the last encounter of any kind, night of the last knock, knock ids heard, last 10) and `encounter = { kind: 'knock', id } | null`.
- **At most one encounter a day:** no knock on a day with `deck.last === day`.
- **A knock** can come when you sleep in the van (not the pullout): odds 0.11 × spot (lot 1.4, truckstop 1.2, trailhead 0.9, ridge 0.6, driveway 0.5), 0 if fewer than 4 nights since the last. Roll: `events`, `knock-{day}`, first draw against the odds; the knock is drawn from the spot's knocks not among the last 3 heard (all of the spot's if none are left).
- **While an encounter is pending,** every action but the answer is refused, and the night hasn't happened.
- **The answer** applies its effects (cash; energy, clamped; supplies; psyche × 0.5, rounded, clamped; bond ± with the driveway's host), says its line, then runs the night as a normal van sleep.
- 10 knocks × 3 answers, by spot; the driveway's is the friend's.

**What it means here:** a seeded, spot-weighted interrupt on the sleep action with a three-way choice, cooldowns, and a shared one-a-day cap that 22.6b's drive events and 22.6c's walk-outs will share.

## 2026-09-30 — Phase 22.5b: busking

**Built in the 2D rebuild's Phase 22.5b** (`dirtbag/app/src/sim/busk.ts`, the beat in `ui/BuskPanel.tsx` and `game/game.ts`; numbers in `BUSK` in `dials.ts`). Evan's calls; numbers [proposed]. Save v19.

**The rules:**
- **A set:** at the café, 08:00–20:00, not on a rain day, once a day (`busked` flag), ≥5 energy. Costs 60 min and 5 energy.
- **The beat** (UI): 8 notes. A marker sweeps 0→1 across a bar in 1.3 s; a tap scores 1 within 0.08 of 0.5, 0.5 within 0.16, else 0; a sweep that ends untapped scores 0. The sim gets `{ t: 'busk', acc }`, acc = mean score, and trusts it (it's the player's hands, like the speed wall's time).
- **Playing:** `guitar` += 0.5 + 0.5 × acc per set. Skill = 1 − e^(−guitar/150). Ranks at skill 0.25, 0.55, 0.85 (busker, regular, local legend), each with a line when reached.
- **Crowd** = hour factor (08:00 0.6, 11:30 1.2, 14:00 0.8, 17:00 1.3) × 1.3 on weekends × sky (prime 1.1, fair 1, hot 0.8) × luck (1 ± 0.2, `events`, `busk-{day}`).
- **Tips** = round((4 + 20 × skill) × crowd × (0.3 + 0.7 × acc)). **Heads** = round((2 + 28 × skill) × crowd).

**What it means here:** a skill-by-repetition side income with a rhythm minigame, starting below every job's hourly pay and ending above all of them, capped at one short set a day so it never replaces a day's work.

## 2026-09-30 — Phase 22.5a: the hustle (cans, bins, foraging)

**Built in the 2D rebuild's Phase 22.5a** (`dirtbag/app/src/sim/hustle.ts`; numbers in `HUSTLE` in `dials.ts`). All numbers [proposed]. Save v18.

**The rules:**
- Three acts, each once a day, the take drawn at the act from `events`, `hustle-{kind}-{day}` (so the same day always gives the same take):
  - **Cans** (the van, by day): 120 min, −8 energy, cash int[3, 9].
  - **Bins** (the market, night only): 60 min, −4 energy; p 0.78 of food int[24, 39], else nothing. Food found counts as the meal `bins` in the last-meals list.
  - **Forage** (the lake, by day): 120 min, −6 energy, food int[18, 29], ×0.4 rounded in winter; the meal `forage`.
- **Teaching:** after any action, if food < 35 and cash < $10 and `seen` lacks `hustle`, one line names all three and `hustle` is added to `seen` (a list of one-time lines, new in v18).
- **Balance rule:** each hustle's best hour, food priced at ramen's cost per point, stays under the worst-paid shift's first-rank hourly pay.

**What it means here:** a small free-money/free-food safety net, gated by time and a daily flag, with a one-time tutorial prompt keyed to a hungry-and-broke state.

## 2026-09-30 — Phase 22.4d: psyche

**Built in the 2D rebuild's Phase 22.4d** (`dirtbag/app/src/sim/psyche.ts`; numbers in `PSYCHE` in `dials.ts`). All numbers [proposed]. Save v17.

**The rules:**
- **Psyche** (0–100): a new climber starts at 60; a migrated one at 50. It's settled at every night (van, ledge or expedition), from the day that's ending:
  - Lifts: +4 for a first send of any line that day; +6 for arriving at a crag you'd never been to, else +2 for any crag day (a ledge or expedition night counts as one); +3 for climbing, watching or belaying with anyone; +2 for sitting at the fire at night.
  - Wears: −3 for a shift worked. A day with no lift is stale; a run of stale days costs 1 less than its length, today included, up to −4 (so 0, −1, −2, −3, −4, −4…). Any lift ends the run.
  - The day's net is capped at ±8. Then it drifts 10% of the way to 50 and rounds.
  - A crag you'd logged a line at before the save moved to v17 counts as visited.
- **Effect:** every window × (1 + 0.05 × (psyche − 50) / 50): 0.95 to 1.05.
- **Words:** under 25 low, under 45 flat, under 60 steady, under 80 keen, else psyched. A line the morning it tips into low or psyched. Tonight shows the word by morning and the day's reasons.

**What it means here:** a slow, bounded mood stat fed by a day's variety and company, pulled back to even every night so no single habit holds it up, with a small multiplier on every skill check.

## 2026-09-30 — Phase 22.4c: supplies and sickness

**Built in the 2D rebuild's Phase 22.4c** (`dirtbag/app/src/sim/sick.ts`; numbers in `SUPPLIES` and `SICK` in `dials.ts`). All numbers [proposed]. Save v16.

**The rules:**
- **Supplies** (0–100, start 80): −8 per van night, +20 per night at the truck stop.
  - Refills: lake +40 (not in winter), market $4 for +30, gym shower +30 (needs a day pass, once a day).
  - Low is under 25.
- **Sickness:** at each van night, if not already sick, roll (`events`, `sick-{day}`) against p:
  - p = 0.01, +0.03 on a winter night without the heater lit, +0.04 on low supplies, +0.04 if food is under 20, +0.03 if the last 4 meals were the same.
  - On a hit, if supplies are low, a second draw under 0.4 makes it a toothache. Otherwise it's a cold (winter) or a bug. A cold or bug lasts 2–4 days.
  - While sick: −20 energy on each night's rest, and every window ×0.85.
  - A toothache persists until a doctor ($30 × the plan's share, 60 minutes). A doctor halves a cold or bug's remaining days (ending it at 1 day or less).
- Propane stays separate: the heater's fuel, by the tank.

**What it means here:** a consumable hygiene/water meter with multiple refill sources, and a nightly illness roll driven by several state conditions, with a lingering debuff.

## 2026-09-30 — Phase 22.4b: old injuries and fear

**Built in the 2D rebuild's Phase 22.4b** (`dirtbag/app/src/sim/scars.ts`; numbers in `SCARS` and `FEAR` in `dials.ts`). All numbers [proposed]. Save v15.

**The rules:**
- **Marks:** when an injury heals, a seeded roll (`session`, `scar-{day}-{kind}`) leaves a mark on its area. The chance is 0 / 0.35 / 0.7 by tier.
  - The area comes from the injury's name: fingers, shoulder, forearm, leg, or ankle for a landing.
  - A mark multiplies injury chance on styles that load that area ×1.3, and highball landing chance ×1.3 for an ankle.
- **Flares:** each go on a marked style without an injury rolls 6% (`flare-{day}-{go}`) to flare the mark for 3 days. Only one flare at a time.
  - While flaring, that area's styles have windows ×0.9.
  - Physio ends a flare.
- **Fear:** a landing or deck injury, or any tier-3 injury on a go, adds the line's style to a fear set.
  - Feared styles have windows ×0.9.
  - Sending any line of that style removes it.

**What it means here:** persistent per-area risk modifiers, a temporary per-area window penalty, and a per-style confidence penalty cleared by a send.

## 2026-09-30 — Phase 22.4a: insurance plans and the clinic

**Built in the 2D rebuild's Phase 22.4a** (`dirtbag/app/src/sim/clinic.ts`; numbers in `PLANS` and `CLINIC` in `dials.ts`). All numbers [proposed]. Save v14.

**The rules:**
- **Plans:** chosen at any time and paid with the weekly bills (registration $45 + premium).
  - None: $0, clinic bills ×2.
  - Catastrophic: $25, bills ×1. This is the default.
  - Full: $45, every bill $20, and physio and cortisone at 25%.
  - Clinic bills are 0/45/210 by tier before the multiplier. The first injury is free.
- **Physio:** 120 minutes, $40 × the plan's share, once a day, with an injury. The injury ends 1 day sooner.
- **Cortisone:** 30 minutes, $60 × the share. It removes floor(days left / 2), and marks the day of the shot.
  - Within 21 days of a shot, the next injury rolled is one tier worse (up to 3).
  - Its time off grows by the gap between the tiers' shortest durations. The mark then clears.

**What it means here:** an insurance setting modulating injury costs, and two treatment actions, one with a delayed penalty.

## 2026-09-30 — Phase 22.3b: the lake

**Built in the 2D rebuild's Phase 22.3b** (`dirtbag/app/src/sim/lake.ts`; numbers in `LAKE` in `dials.ts`). All numbers [proposed]. No save change.

**The rules:**
- **Fishing:** 120 minutes and −6 energy, once a day.
  - Three tries at a fish, each with a chance p.
  - p is 0.4 in spring, 0.5 in summer, 0.5 in fall and 0.15 in winter. It's ×1.5 before 9:00 or from 17:00, capped at 0.9.
  - The roll is seeded from `events`, `fish-{day}-{minute cast}`.
  - Each fish is +15 food, eaten at once. No cash.
- **A swim:** 45 minutes, +8 energy, once a day, not in winter.

**What it means here:** a seeded yield activity whose output is food, bounded below a meal's worth, with season and time-of-day modifiers.

## 2026-09-30 — Phase 22.3a: the pantry and the kitchen

**Built in the 2D rebuild's Phase 22.3a** (`dirtbag/app/src/sim/content/food.ts`, `game.ts`, `climb.ts` `dayFactor`; numbers in `FOOD` in `dials.ts`). All numbers [proposed]. Save v13.

**The rules:**
- **The pantry** counts servings per ingredient. The Market sells packs of 4 servings: oats, rice, beans, tortillas and pasta at $4; eggs, cheese and greens at $6.
- **Recipes** need the camp kitchen (an owned upgrade, $80) and one serving of each ingredient:

  | Recipe | Ingredients | Time | Food | Other |
  |---|---|---|---|---|
  | Oatmeal | oats | 15 min | +30 | +4 energy |
  | Rice and beans | rice, beans | 30 min | +45 | |
  | Burritos | tortillas, eggs, cheese | 30 min | +50 | fuels |
  | Pasta | pasta, greens | 40 min | +45 | +3 skin, fuels |

- **Fueled:** the day a fueling meal is eaten, every crux window ×1.05 (inside the day factor). Nothing carries over, and no skill is gained.
- **Variety:** the last 4 meals are kept (recipes, ramen, the diner special). When all 4 match, it's shown on the night summary. 22.4's sickness will read it.
- **Coffee:** the third and later cups in a day give −6 energy instead of their normal gain.
- **Hungry bedtime:** if food is under 20 at bed, eat up to 2 servings of anything in the pantry, +15 food each, until it's 20 or more.

**What it means here:** an inventory of ingredients, recipes as data, a per-day buff multiplier in the window model, and a short meal history.

## 2026-09-30 — Phase 22.2c: van upgrades and winter

**Built in the 2D rebuild's Phase 22.2c** (`dirtbag/app/src/sim/spots.ts` `nightAt`, `van.ts`, `content/gear.ts`; numbers in `UPGRADE` and `WINTER` in `dials.ts`). All numbers [proposed]. No save change.

**The rules:**
- **Upgrades** are owned items, each fitted once at the garage for a price and a time:
  - bed: $70, 60 min, +5 energy each van night (not in the pullout);
  - curtains: $60, 30 min, Lot ticket odds ×0.3;
  - tool kit: $85, 5 min, bodge odds 0.9 instead of 0.6;
  - tune-up: $90, 90 min, gas ×0.8, rounded;
  - heater: $45, 90 min (was $150; Evan's call, so it's affordable before the first winter on day 15);
  - insulation: $60, 120 min, cold ×0.5, rounded.
  - None produces income.
- **Winter nights:** in the winter season (the second 14 days of every 56), a van night's cold is −10 plus the spot's winter value. It's not the spot's in the pullout.
  - With a heater and propane > 0, the cold is 0 and one propane is used.
  - Otherwise, insulation halves the cold.
- **Propane:** a consumable, $18 for 10 at the gear shop.
- **The warning:** a line on the night before day (winter start − 5).

**What it means here:** the van carries owned modifiers, and the night's rest gets a seasonal cold term with a fuel-burning counter.

## 2026-09-30 — Phase 22.2b: where you sleep

**Built in the 2D rebuild's Phase 22.2b** (`dirtbag/app/src/sim/spots.ts`, `game.ts` `sleep`; numbers in `SPOTS` and `SPOT` in `dials.ts`). All numbers [proposed]. Save v12.

**The rules:**
- A standing choice of spot, applied on every night at the van. Each spot has a price, gas, a morning drive (minutes added to the wake time), and an energy change, with an extra winter change:

  | Spot | Price | Gas | Drive (min) | Energy | Extra in winter |
  |---|---|---|---|---|---|
  | The Lot | $18 | $0 | 0 | 0 | 0 |
  | Upper Trailhead | $0 | $6 | 25 | −10 | −10 |
  | Truck stop | $6 | $2 | 10 | −12 | 0 |
  | Friend's driveway | $0 | $2 | 10 | +5 | 0 |
  | The Ridge | $0 | $8 | 40 | +5 | −15 |

- **Tickets at the Lot:** with n nights in a row there so far, the chance tonight is min(0.25, 0.05 × (n + 1 − 3)), never below 0. The fine is $25, on the card. The roll is seeded from `events`, `ticket-{day}`. Any other spot resets the count.
- **The driveway:** needs a partner at bond tier 3 or more who isn't away, and 4 days since the last visit.
- **The Ridge:** needs 25 trips out.
- **Fallback:** a spot that won't have you tonight becomes the Lot. A card that can't cover the price and gas is the pullout: nothing paid, no drive, the rough night's energy.
- You always wake at the Lot. Lifestyle is paid after the spot.

**What it means here:** the night gets a location with its own costs and rest, plus a consecutive-nights counter for tickets.

## 2026-09-30 — Jobs pay in more than money (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/content/jobs.ts`, `content/places.ts`, `jobs.ts` `tipsFor`).

**The rules:**
- A job that pays less an hour trains something: coaching +2 head and +1 technique a shift, setting +3 technique, the warehouse +2 endurance. The café and the diner train nothing.
- The café pays least a shift ($28) and has the shortest shifts (3 h). Setting went from $28 to $30 to keep that true.
- A new job, the Diner: 4 h at $30 base, raise $6 a rank, ranks Busser, Server, Head server, Floor manager at 0/10/24/42 shifts, and 5 shifts posted a week.
  - Tips on top: a seeded integer from 4 to 10 (`events` stream, `tips-{job}-{day}`), ×1.5 rounded on a weekend.

**What it means here:** a job's reward is pay plus a flat skill gain, and a job can carry a seeded tip roll.

## 2026-09-30 — Phase 22.2a: the van's parts, breakdowns and the garage

**Built in the 2D rebuild's Phase 22.2a** (`dirtbag/app/src/sim/van.ts`, `game.ts`, `content/van.ts`; numbers in `VAN` in `dials.ts`). Three parts by Evan's call (Decision 5); the numbers are [proposed]. Save v11.

**The rules:**
- **Parts:** tires, engine and battery, each 0–100. A new climber's van starts at 85; a migrated one at 70.
- **Wear:** tires lose 0.015 and the engine 0.01 per minute driven, and the battery 1 a night. A climber driving to the nearest crag every day needs tires about every 55 days and an engine service about every 80: about $5 a day, battery included. At 0 the battery needs a jump: drives across town only (under 20 minutes), plus home and the garage, so town and a shift are always in reach.
- **Breakdowns:** only on drives of 20 minutes or more. The chance is (minutes / 60) × 0.14 × w², where w is the worst road part's wear (0 new, 1 dead), capped at 0.5. A van kept up never breaks down.
  - The roll is seeded from the `events` stream, derived `breakdown-{day}-{minute}-{from}-{to}`.
  - The part that goes is picked weighted by wear + 0.05, and drops to 5.
  - The breakdown happens halfway: half the drive's wear and time, then nothing else can happen until you take a way out.
- **Ways out:**
  - **Tow:** $70 and 90 minutes, to the garage.
  - **Bodge:** 60 minutes, holds 60% of the time (its own seeded roll). If it holds, the part goes to 35 and you drive on. One try per breakdown.
  - **Limp on:** the rest of the drive at double time and −15 energy. The part stays at 5.
  - **Friend:** a partner at bond tier 2 or more who isn't away. 120 minutes, the part to 30, and you drive on.
- **Unsafe:** a road part under 15 refuses any drive of 20 minutes or more, except home to the Lot or to the garage.
- **The garage (Midtown):** each part back to 100 for its price × wear (tires $140, engine $180, battery $90, never under $15), in 60, 120 and 20 minutes.

**What it means here:** the van becomes state with three condition meters, a travel-time breakdown roll and a blocking breakdown state with four resolutions. Parking spots, upgrades and winter come in 22.2b and 22.2c.

## 2026-09-30 — Phase 22.1: the week, the jobs and how you live

**Decided by Evan (jobs differ, start low, promotion to earn more; a warehouse job added); built in the 2D rebuild's Phase 22.1** (`dirtbag/app/src/sim/content/jobs.ts`, `jobs.ts`, `game.ts`; numbers in `WORK` and `LIFESTYLE` in `dials.ts`). Save v10.

**The rules:**
- **Four jobs**, each with a shift length, a first-rank pay, ranks by shifts worked, and a raise per rank (top rank 1.7 to 2 times the first):
  - Coffee Shop: 3 h, $28, raise $5, five ranks at 0/12/30/54/84 shifts. Posts every day.
  - Coaching (The Cave): 4 h, $34, raise $17, three ranks at 0/8/20 shifts and V5/V6/V8. Posts 4 days a week.
  - Setting (Send City): 4 h, $28, raise $10, four ranks at 0/6/16/30 shifts and V0/V3/V5/V7; trains technique. Posts 4 days a week.
  - Warehouse [proposed]: 8 h from before 9 AM, $52, raise $17, four ranks at 0/10/25/45. Posts 3 days a week.
- **Posting:** a job that doesn't post every day picks its days per 7-day week off the `worldgen` stream, derived `shifts-{job}-w{week}` (partial Fisher–Yates over the week's days). Day 1 is a Monday.
- **Sign-up:** a posted shift from tomorrow up to 7 days ahead, one shift a day, the job's first-rank grade needed. Today's posted shifts are walk-ins: they pay the same and don't count toward promotion. Only a shift you signed up for counts.
- **No-shows:** a signed-up shift not worked by the end of its day is a warning. The third costs the job: shifts there reset to 0, future sign-ups there dropped, and 7 days before it takes you back.
- **How you live:** dirtbag (free), comfortable ($12 a night, +8 energy, +4 skin by morning), plush ($28, +15, +8) [proposed]. Paid at the van after the spot, only while the card covers it; otherwise that night is a dirtbag's. Not on a ledge or an expedition.

**What it means here:** jobs become data with a weekly posting draw, the save carries a shift schedule, warnings, a let-go date per job and a lifestyle, and sleep settles no-shows and the lifestyle's cost.

## 2026-09-30 — The Cave raised, and the Training Center (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/content/gym.ts`, `content/places.ts`).

**The rule:**
- The Cave's weekly set is now V5 to V12 (v0.956's V3 to V10, raised).
- The Training Center, v0.956's third gym: eight comp-style problems a week, V7 to V14, seeded by week. Styles weighted dyno 3, technical 2, power 2, crimp 1 in 8. Its own day pass ($20 [proposed], against Send City's $14); specialty power and technique (×1.2, as the other gyms' are). You need V7 to go there, like Sandstone Mesa.
- Phase 21 is closed: with these, the 2D game's career bots reach V10 in every run and never run out of things to try.

**What it means here:** a third gym level: a comp hall, and a weekly set generated from its own table.

## 2026-09-30 — Phase 21.6: the speed wall, and Free Solo

**Built in the 2D rebuild's Phase 21.6** (`dirtbag/app/src/sim/speed.ts`, `solo.ts`, `game.ts`; numbers in `SPEED` and `FREESOLO` in `dials.ts`). Save v9.

**The speed wall** (Send City, with the day pass):
- A run is 20 holds. The start is three lights a second apart, go on the third; a grab before it is a false start. Then alternate hands; the same hand twice is a slip that loses 0.35 s.
- The clock time = the player's real seconds (at least 1.5) × a factor: 2.8 at V0, minus 0.1 per grade of (0.6 power + 0.4 technique), never under 1.45. A PB is kept.
- A run costs 10 min, 7 energy, 2 skin and 2 food, and loads you like a short go (×0.8). The first 3 runs a day (false starts count) teach power 0.14 and technique 0.06 of a training session's base, each slowed by hi(); after that, nothing.

**Free Solo** [proposed]:
- Chosen at creation, and never changed afterwards.
- Every outdoor sport line and wall pitch is climbed without a rope: no belayer or rope needed, and every crux window ×0.82. Trad and boulders are unchanged.
- A solo send teaches head +2. A fall ends the run: the state is marked dead and nothing else can happen to that climber.
- The state records the solo in progress from the moment you pull on. Loading a state with a solo in progress resolves it as a fall, so reloading can't undo it.

**What it means here:** the sim needs a mode flag, a "dead" end state and a solo-in-progress marker in the save. The speed wall is a separate minigame with its own time model.

## 2026-09-30 — Phase 21.6: highballs by height (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/content/routes.ts` `isHighball`, `HIGHBALL.fromFt` in `dials.ts`).

**The rule:** every outdoor boulder 16 ft or taller is a highball; deep-water solos and gym problems never are. It isn't set by hand. The landing rule itself is unchanged (Phase 10.3b): nothing up to 8 ft of fall, +1.2% a foot above that, halved by the haul's pads and again by a spotter. Seventeen problems qualify, twelve of them new.

**What it means here:** the highball flag is derived from height, so the Unreal content pipeline can compute it rather than author it.

## 2026-09-30 — Phase 21.6: crowds and spray beta

**Built in the 2D rebuild's Phase 21.6** (`dirtbag/app/src/sim/crowds.ts`; numbers in `CROWD` in `dials.ts`, each crag's draw as `crowd` in `content/places.ts`). No save change: a crowd is worked out, not stored.

**The rule:**
- A crag's crowd level = its draw (0.2 at The Crucible to 0.6 at Roadside and The Big Stone) × 1.7 at the weekend × the hour (0 before 7:00 and from 19:00; 0.5 from 7:00, 1 from 9:00, 0.7 from 16:00) × the sky (prime 1.25, fair 1, hot 0.6; wet or closed rock 0) × the day's luck (0.7–1.3, seeded per crag and day). Read off as empty (<0.25), quiet, busy (≥0.7) or packed (≥1.2).
- A queue before each go: busy, 10 min for a rope and none for a boulder; packed, 20 and 10. A wall's pitches have none.
- Asking around (busy or packed, a line you haven't sent, beta you don't know): 15 min, and you learn the line's next unknown beta as told, so a first-go send is a flash.
- Packed, on your first go on a line: a 40% seeded chance someone shouts that beta at you before you start. The same outcome.

**What it means here:** crowds at the crag base to stage, and a queue that takes time.

## 2026-09-30 — Phase 21.5: walls and expeditions

**Built in the 2D rebuild's Phase 21.5** (`dirtbag/app/src/sim/content/routes.ts`, `content/expeditions.ts`, `expeditions.ts`, `game.ts`; numbers in `WALL` and `EXPED` in `dials.ts`). Save v8.

**Walls:**
- v0.956's four, pitch by pitch, as sport pitches of 100 ft and 16 moves: The Long Prow at Roadside (4 pitches, sport grade 5; v0.956's "The Prow", renamed because the Mesa's boulder has the name), Golden Buttress (5, grade 9), Obsidian Tower (7, grade 13) and The Ascendant (8, grade 18) at The Big Stone.
- Needs a rope of your own ($150 at the gear shop) and dry rock. You start the wall, then climb its pitches in order: each is a normal go through beta-then-send, costing 60 min, 8 energy, 4 food. A fall leaves you at the same pitch.
- After dark, once off the ground, you can bivy on the ledge: no van fee, +40 energy, −20 food, and you wake on the wall. Rapping off or driving away ends the attempt; the next one starts at pitch 1.
- The first summit pays 40 + 12 × grade + 10 × pitches ($140 for the Prow); later summits pay nothing.

**Expeditions:**
- v0.956's three: El Capitan (V7 to go, 6 pitches in 10 days, storms 18%, $900, pays $2,400), Cerro Torre (V10, 8 in 16, 44%, $2,200, $6,000), Trango Tower (V12, 11 in 24, 56%, $4,500, $12,000). Paid in cash from the Lot; nothing else happens while you're away.
- Each day is one choice: lead (−22 energy), dig deep (−36), rest (+48), or bail. A storm day (seeded per expedition and day) allows only rest or bail. Every night gives +10 energy, up to 100.
- A pitch succeeds with p = clamp(0.45, 0.97, 0.68 + 0.1 × endurance margin over the objective's grade (+0.25 digging deep)), rolled on a seeded stream per day and pitch.
- The summit's odds are shown before you pay and every day after: an exact calculation over storms and falls, for leading every fair day and for digging deep every one. For a V9 climber on El Cap that's about 48% and 81%.
- The summit pays and trains head +2. Running out of days or bailing pays nothing, and the cost is spent.

**What it means here:** walls reuse the attempt as-is, so they need a wall-length scene with ledges and an overnight on the rock. An expedition is a turn-based layer over days, with its odds in the UI.

## 2026-09-30 — Phase 21.4: The Cave, a second gym

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/gym.ts`).

**The rule:**
- Indoor places are a set, each with its own day pass, closing time (10 PM) and specialty: Send City brings on technique and endurance ×1.2, The Cave power and fingers ×1.2.
- The Cave sets eight problems a week, V3 to V10, seeded by week, drawn mostly from power and crimp styles.
- Coaching there: 4 hours, $34, trains head +2 and technique +1; needs V5; ranks at 8 and 20 shifts (V6, V8), +$8 a shift each.

**What it means here:** a second gym level with its own weekly set and job.

## 2026-09-30 — Phase 21.4: Psicobloc Cove, and deep-water solo

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/routes.ts`, `game.ts`, `weather.ts`).

**The rule:**
- A route may be a deep-water solo (`dws`): a boulder problem over the sea. A fall is into the water from wherever you were: no pad needed, no landing roll, no strain (overuse) roll. It still loads you and costs skin like any go.
- A place may be closed for several seasons: the Cove is open in summer only ("Cold, rough seas till summer").
- The Cove: V4, two hours from the Lot, no fee, its own weather. v0.956's eight lines: Tide Pool Traverse V2, The Plunge V3, Saltwater Slab V4, Barnacle Crimps V5, Leap of Faith V6 (climbs V7), Overhanging Tide V7, Psicobloc Arête V8, The Deep End V9.

**What it means here:** a fall resolution with no injury path, and a water landing to stage.

## 2026-09-29 — Phase 21.4: The Crucible, and myths

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/routes.ts`, `game.ts`).

**The rule:**
- A crag opening at V11, four hours from the Lot, shaded, with its own weather and no fee. v0.956's lines: The Reckoning V13, The Vise V14, Apparition V15 (climbs V14), Event Horizon V17; Crucible Crux 5.14b, The Lifeline 5.14d, Threshold 5.15d. Added: The Anvil V16, so every grade V0–V18 has a line.
- A route may name another it's hidden behind (`hiddenUntil`). Until that one is sent, the hidden line can't be read or tried: the V18 boulder myth behind Event Horizon, the 5.16a sport myth behind Threshold. Both are open (unclimbed); the first to send one names it.

**What it means here:** a route's visibility and availability depend on another route's send; the 3D wall should show nothing on a myth until then.

## 2026-09-29 — Phase 21.4: Wind River Walls

**Built in the 2D rebuild's Phase 21.4.** Content, not a new rule.

**The rule:** a crag opening at V9 after an $800 trip paid once, with a $35 permit every trip; four hours from the Lot, reached past the Gorge; shaded, its own weather, closed in winter. v0.956's nine lines: boulders Alpine Crimps V9, The Diamond V11 (climbs V10), Offwidth Horror V12, Thin Air V13, an open V15; sport Glacier Point 5.13c, Skyline Traverse 5.14a, Astroman 5.14c; trad Alpine Trad 5.14b. A partner needs bond 7.

## 2026-09-29 — Phase 21.4: The Big Stone

**Built in the 2D rebuild's Phase 21.4.** Content, not a new rule.

**The rule:** a crag opening at V8 after a $600 trip paid once, four hours from the Lot, shaded, with its own weather. v0.956's five single pitches: Base Camp Boulder V6, The Warm-Up Wall V9 (climbs V10), The Splitter Pitch V10 (highball), The Trad Pitch 5.13d (trad), Valley Classic 5.14a (sport). A partner needs bond 7 to come out. Its multi-pitch walls come with Phase 21.5.

## 2026-09-29 — Promotions, and inviting crew to any crag (decided)

**Decided by Evan** after the harness found bots going broke at V5; built in the 2D rebuild (`dirtbag/app/src/sim/jobs.ts`, `content/jobs.ts`, `content/people.ts`). Save v7.

**The rule:**
- Each job counts shifts worked (a double is two). A rank comes at a shift count and, for setting, a grade: café ranks at 12, 30, 54, 84 shifts, +$4 a shift each; setting at 6, 16, 30 shifts and V3, V5, V7, +$7 a shift each. The shift that earns a rank pays the old rate.
- Each crag carries the bond a partner needs to come out there when asked: Roadside 1, the Gorge 3, the Mesa 5, Moonstone 5. Asking is once a day, before 1 PM, to Hazel or Sage; the crag must be dry, your grade must open it, and a paid-for crag must be paid for. They're there for the day from the minute you ask.

**What it means here:** partner presence gains an invite override for every partner, not just Sage; jobs gain a rank ladder the 3D game's work screens need to show.

## 2026-09-29 — Phase 21.4: Sandstone Mesa

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/routes.ts`, `places.ts`). Content, not a new rule.

**The rule:**
- A crag opening at V7, three hours from the Lot, with no unlock fee; desert (windows ×0.92), its own weather, closed in summer.
- v0.956's eleven lines, names and grades kept: boulders Desert Varnish V7, Sandstone Crimps V8, The Prow V9 (climbs V10), Desert Splitter V9, Powerhouse V10, The Megaproject V11, and an open V13; sport Desert Lap 5.13a, Desert Enduro 5.13b, The Big Link 5.13d; trad Desert Trad Line 5.13d.
- Sage can be invited there to belay, as to the Gorge (bond 3, V7, the crag open that day).

**What it means here:** the Mesa is a level with the same route data. v0.956's multi-pitch "The Prow" at Roadside (Phase 21.5) will need another name, since the Mesa's boulder has this one.

## 2026-09-29 — Phase 21.3: training

**Built in the 2D rebuild's Phase 21.3** (`dirtbag/app/src/sim/training.ts`, `sessions.ts`, `content/training.ts`, numbers in `TRAIN` in `dials.ts`). Save v6.

**The rule:**
- Six protocols, each with a place (the van with a hangboard, or the gym with a pass), a cost (minutes, energy, food, skin), two skill weights, an injury style and an intensity: max hangs, repeaters, pull-ups and core (van); campus from V4, 4x4s, ARC (gym). Plus prehab at the van.
- A session teaches `(2 + 0.35 × grade) × phase × (0.6 if the load ratio is over 1.5)`, split by weight, each skill through `hi()`. One a day.
- It loads you like a go: `goLoad(energy, grade, 1) × intensity`, and rolls for injury on the same model as a go, with its style's risk.
- Refused while tapering, injured (prehab excepted), fried (ratio over 1.7), empty, or short of the session's energy.
- Phases: base (×1 everything; no lock), build (sessions ×1.25, injury ×1.2), peak (every crux window ×1.04, sessions ×0.7, injury ×1.3), deload (sessions ×0.5, injury ×0.6, acute load ×0.8 each night). A chosen phase holds 6 days; peak turns into deload after 7.
- Taper: 3 days with no training, windows ×1.03 then ×1.06 on day 3; 14 days from its end before the next.
- Prehab: injury chance ×0.6 for 8 days.
- Target: at every grade reached, the best protocol's skill an hour stays under climbing's.

**What it means here:** training is a sim action beside `ResolveAttempt`, with the same load and injury model. The phase and taper scale the per-move windows alongside skill, conditions and kit.

## 2026-09-29 — Phase 21.2: trad

**Built in the 2D rebuild's Phase 21.2** (`dirtbag/app/src/sim/climb.ts`, `content/routes.ts`, numbers in `TRAD` in `dials.ts`). No save change.

**The rule:**
- Trad is a third discipline beside boulder and sport. A trad line has stances (in moves) instead of bolts: one low, then about every 3.2 moves, one at the rest, and none inside a crux.
- Leading trad needs a rack (owned kit, $280, or 55% at the swap meet) and a belayer. A go costs 35 minutes, 12 energy and 7 food.
- Between cruxes, letting go within a stance's reach (0.5 moves below it to 0.8 above) places a piece after 1.1 s. While placing, pump rises 2/s where hanging would pay back 4.5/s. Climbing on before 1.1 s places nothing. Climbing through a stance runs it out.
- A fall catches on the highest piece placed below you, exactly as sport's does on the last bolt clipped: fall = 2 × distance over it + 3 ft of slack.
- If that fall is at least the height you're at, you hit the ground: a deck of that height. Nothing under 6 ft hurts; above it, each foot adds 3.6% to an injury roll (ankle names; tiers from 6 and 12 ft over), with no pads or spotter to reduce it.
- A trad send adds 0.4 × the go's learning rate to head, on top of what the go teaches.
- Pieces never pull, and every piece holds the same.

**What it means here:** `ResolveAttempt` needs protection as part of the attempt's state: the moves where pieces went in, chosen during the go, not before it (v0.956's single rack choice is gone). Fall length and the deck check read it. In 3D, a stance is a spot on the wall where the climber can stop, and placing is an animation the player waits through while pump climbs.

## 2026-09-29 — Phase 21.1: your kit

**Built in the 2D rebuild's Phase 21.1** (`dirtbag/app/src/sim/kit.ts`, `content/gear.ts`, numbers in `KIT` in `dials.ts`). Save v5.

**The rule:**
- The state carries a kit: a number per item. Shoes have a condition (0–100); chalk and tape have uses left; a second pad is owned or not.
- A new climber starts with worn shoes (60) and half a bag of chalk (30 goes). The rope on a sport go is still the belayer's.
- Every go wears the shoes by `0.25 × (1 + 0.035 × grade)`, uses one go of chalk, and a wrap of tape on a crack if you have tape.
- The kit scales every crux window, in the same product as skills and the day's conditions (and so the windows the player sees before a go): shoes under 40 ×0.95, under 15 ×0.87, the penalty doubled on technical lines; no chalk ×0.95.
- Tape halves the skin a crack costs, the go's base cost included. A second pad halves a highball's landing risk, as a paid haul's pads do.
- A gear shop sells a resole (to 90, only when under 90), new shoes (100), chalk, tape and a pad; a swap meet on weekends (the last two days of each seven) sells used shoes (60) and pads at 55%.

**What it means here:** `ResolveAttempt`'s per-move execution windows take the kit factor alongside skill and conditions, and a go's outcome writes the kit's wear back. The 3D game needs somewhere to buy and see the kit; the numbers are the 2D game's dials.

## 2026-09-29 — Phase 12.1: the journal, and long lines on a card

**Built in the 2D rebuild's Phase 12.1** (`dirtbag/app/src/ui/Sheet.tsx`, `src/game/game.ts`). A UI feature, not a rule: the sim's log is unchanged.

**The rule:**
- The sim's log (the last 200 lines, in the save) is the player's message log, shown newest first, by day, beside the climber's own page.
- No transient notice runs over 30 words. A longer line goes on a card the player dismisses, which waits until nothing else is open.
- The navigation Phase 12 proposed (five tabs) is dropped: scenes and the map stay the way in, and the journal is the one place for the rest.

**What it means here:** whatever the 3D HUD does for notices, the same cap applies, and the log is a view of the sim's log, not a second one.

## 2026-09-29 — A send after the first is a repeat (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/game.ts`, the `done` action).

**The rule:**
- A line's first send is an onsight (first go), a flash (first go, having been told the beta) or a redpoint (a later go). The route log keeps that one.
- Any send after the first is a **repeat**. The 2D rebuild had named every send after go one "Redpoint", sends of lines already done included; v0.956 called them repeats, as climbers do.
- A repeat gets no send card and can't be a first ascent. It already trained less (a lap of a sent line teaches less), and that doesn't change.

**What it means here:** send styles are four, not three, in whatever reports a send; the stored first-send style stays three. No save change in the 2D game.

## 2026-09-29 — Phase 11.4: the daily plan

**Built in the 2D rebuild's Phase 11.4** (`dirtbag/app/src/game/plan.ts`; the runner in `src/game/game.ts`). It's a UI feature, not a rule: each step runs through `act()` as a tap would.

**The rule:**
- A plan is a list of steps: a place, and an act there or a climb. It runs in one go: drives, then acts. It waits at a climb while you climb, and stops at the first thing the day refuses, saying why. Bed ends it.
- Yesterday, as played, is the default plan: every act, and a climb once a visit.
- What can be planned comes from the data: a place's acts, and climbing where there's something to climb.
- It's stored beside the save, not in it.

**What it means here:** the day's logistics (shifts, meals, drives, bed) can be one decision, and the climbing stays the part you play. For "2D minigames, 3D staging", that's a shape worth keeping: plan the day, then play the session.

---

## 2026-09-29 — Phase 11.3: roads are a graph

**Built in the 2D rebuild's Phase 11.3** (`dirtbag/app/src/sim/content/places.ts`, `ROADS` and `road`).

**The rule:**
- Each place has roads to its neighbours only, and a drive anywhere else is the quickest way through them: least time, then least gas. Adding a place means adding its neighbours' roads, not one to every other place.
- The valley's network, 11 roads:
  - town's six streets;
  - the highway to Roadside from the Lot, the café and the diner;
  - the Gorge's dirt road (60 min, $10) and the desert road to Moonstone (120 min, $18), both leaving the highway at Roadside.
- Kept: every daily trip, and v0.956's drives from the Lot: Roadside an hour and $12, the Gorge two hours and $22, Moonstone three hours and $30.
- Changed: nine rare trips to or from the Gorge or Moonstone, by −10 to +15 minutes and −$4 to +$1. Roadside to the Gorge is the biggest: an hour and $10, was 70 minutes and $12.

**What it means here:** if the Unreal version has places joined by roads (a map, road trips), model the network the same way, as edges between neighbours with a shortest-path trip. A hand-typed table for every pair doesn't scale, and it drifted into numbers no road network could produce.

---

## 2026-09-29 — Phase 11.2: place cards say who's around when you'd get there

**Built in the 2D rebuild's Phase 11.2** (`dirtbag/app/src/sim/presence.ts`, `staysAt` and `knows`; `src/ui/who.ts`; `src/view/header.ts`). It sits on top of the existing presence rules: nobody's schedule changed.

**The rule:**
- A place card reads the place at the hour you'd arrive: now if you're there, otherwise now plus the road's minutes.
- It lists everyone who'll be there between then and the end of the day, each with from and till. A second stay the same day is listed after the first.
- Someone you haven't met shows as "someone you haven't met". Hazel is known from the start (`PersonDef.known`).
- At a place with sport lines or highballs, one line says what that means: a belayer and/or a spotter (a partner there), till when (overlapping partners joined), from when, or nobody.
- Every card has a header: the place's scene at that hour, or a front drawn for a place without a scene.

**What it means here:** a destination the Unreal version offers (a map pin, a road-trip pick) should read the presence functions at the arrival time, not the current one. Whether there'll be a belayer or a spotter is the question to answer before a drive.

---

## 2026-09-29 — Phase 11.1: bed once it's dark, and lying around in one tap

**Built in the 2D rebuild's Phase 11.1** (`dirtbag/app/src/sim/dials.ts`, `DAY.nightFrom`; `content/places.ts`, the `until` field on acts; `game.ts`, `actCost`). It is *[proposed]*, pending Evan's call.

**Why:** the rebuild's e2e bot now counts a player's taps. A day-3 loop (a shift, the crag, three goes, back, dinner, bed) took 32. 12 of them were spent waiting for bed at 6 PM after getting home at 1 PM: an hour of lying around per tap, then a walk to the fire to fill the last hour. Phase 11's budget is 20.

**The rule:**
- **Bed opens at dark**, 5 PM, when the Lot turns to night and the fire's lit. It was 6 PM, "so a bad day is harder to skip". It only made one harder to skip by taps. A skipped day still costs the $18 spot, a shift not worked and a day of the season.
- **"Lie around till dark"** takes the whole afternoon in one act, at the old rate of 4 energy an hour. An act can now run till a time of day, with its cost given per hour and prorated.
- Sleep is unchanged: you still wake at 7:10, and a night still adds to your energy rather than setting it.
- The 2D harness doesn't move: all four season targets pass with the same figures.

**What it means here:** the day's end is a player's choice once it's dark, with one action to get there from any afternoon. If the Unreal version gets a time-skip, it should work the same way: one action to the next beat, at the same per-hour rates as waiting, never a loop of small waits.

---

## 2026-09-29 — The board runs a grade stiff (decided)

**Evan's call**, closing the question the 10.1 entry left open. Every board problem at Send City climbs like the grade above its label: `trueGrade = grade + BOARD_STIFF` (1), as real boards run.
- On your first go it says so, with its own line rather than the crags' sandbag one: "Board grades: that's no V5. Nobody on the mats is surprised."
- The 2D harness: with the V7 now a month-two project, 0 runs in 144 have a day in the first month with nothing new to try.
- The season targets pass. A few runs don't tie into a V5 by day 28, because the board's V5 climbs like a V6.

**What it means here:** the generator from the 10.1 entry gains one field, `trueGrade = grade + 1`. Board grades are labels; the sim climbs the true grade.

---

## 2026-09-29 — Phase 10 follow-up: Moonstone's own sky

**Built in the 2D rebuild** (`dirtbag/app/src/sim/weather.ts`, `skyAt`; the place flag `ownSky`). This follows v0.956, which rolled every crag's weather from its climate.

**The rule:**
- **Valley places** (the Lot, town, Roadside, the Gorge) keep the one valley sky, `skyOn`. R2's season was tuned on it, though v0.956 rolled the Gorge apart.
- **Moonstone is out of the valley**, so it rolls its own sky:
  - the season's odds, shifted by v0.956's desert terms: hot +0.12, rain −0.10 (floored at 0), prime −0.02;
  - rolled on `worldgen`, derived `sky-{place}-{day}`;
  - day 1 is prime everywhere, as in the valley.
  - (v0.956's shaded terms, rain +0.08 and hot −0.06, are in the function for a future shaded crag out of the valley.)
- **Its conditions** (open, seeping, when the sun comes, windows) come from its own sky. v0.956's desert after-rain nuances ("soft after rain", "washed clean") aren't carried: it seeps the day after rain as the valley does.

**What it means here:** a crag's weather is either the region's or its own. This repo can take the same split: one valley sky, plus per-destination skies for road trips, so a wet day at home can be a dry one three hours away.

---

## 2026-09-29 — Phase 10.3b: highballs, and landings that can hurt

**Built in the 2D rebuild's Phase 10.3b** (`dirtbag/app/src/sim/game.ts`, `landingChance` and `rollLanding`; the `HIGHBALL` dials; the `highball` route flag). All of it is *[proposed]*. v0.956 had no highball rule, only a generic close call for climbing without a pad.

**The rule:**
- **Which boulders:** tall ones carry a `highball` flag. Roadside's Highball Arête (18 ft), and Moonstone's Tall Arête (22), Splitter (20) and project (22).
- **How far you fall:** `heightFt × moves reached / moves`.
- **The chance of a bad landing:**
  - Up to `safeFt` (8), it's a normal boulder fall.
  - Above that, the chance is `perFoot` (0.012) × the feet over.
  - × `pads` (0.5) if you own the Moonstone haul's pads.
  - × `spotter` (0.5) if a partner is there (the same check as a belayer).
- **When it rolls:** only on a failed go, and only if the go's overuse roll didn't already hurt you. It uses the `session` stream, label `landing-{day}-{n}`, where n numbers the go within the day as the injury roll's does.
- **What it costs:** an ankle, by feet over the safe height: jammed (tier 1), rolled (tier 2 from 6 ft over), broken (tier 3 from 12). The days off and clinic bills are the injury table's.

**The 2D harness:**
- Careful bots wait for a spotter while a crux fall is worse than 1 in 20.
- With Hazel spotting at Roadside, 7% of moderate first months now end with a jammed ankle (0% before). Reckless bots: 10%.
- The season targets pass.

**What it means here:**
- In a watched 3D session, a highball's height is visible, and so is the fall. This rule gives the sim a stake to show when the climber comes off high: the landing is decided by the sim, and the staging acts it out.
- Pad placement isn't in, because the 2D boulders have one crux each. If this repo's boulders get two places to fall, placement becomes a real choice.
- No new seed streams (a new label in `session`). No change to `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — Phase 10.3a: Moonstone Boulders, and trips you pay for once

**Built in the 2D rebuild's Phase 10.3a** (`dirtbag/app/src/sim/content/places.ts`, `content/routes.ts`, the `unlock` and `travel` actions in `game.ts`, save v4 in `save.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, Phase 10, "As built (10.3a)".

**The place**, on v0.956's terms:
- **Access:** desert quartzite, open at V6.
- **The haul:** $400, paid once in cash in hand (not on the card), opens the trip for good. That's v0.956's `unlockCost`. In the rebuild's words it buys "pads, water jugs and a guidebook".
- **The permit:** $20 on every trip in (v0.956's `permit`). Gas can go on a maxed card; the permit can't.
- **Distance:** three hours and $30 of gas from the Lot, two hours from Roadside.
- **Desert rock:** every window is scaled by `CLIMB.desertFactor` (0.92), v0.956's desert −0.08 on the odds translated the same way seeping's −0.07 was.
- **Sun:** the sun crosses it as at Roadside (the 10.2 entry), with the far project first and the arête by the van last.

**The lines** (v0.956's nine):
- Boulders:
  - Tall Arête V6 (v0.956 named it Highball Arête, like a Roadside line; renamed so logs can't mix them up);
  - Moonstone Mantel V7, The Egg V8;
  - Hueco Pockets V9, which climbs at V8 (`trueGrade`), as in v0.956;
  - Moonstone Splitter V8 (crack), Lunar Roof V11;
  - an open V12 project.
- Sport lines on a spire: Desert Spire (grade 8, 5.13a) and Moonlight Arête (grade 10, 5.13c).
- Each one uses the shared one- and two-crux templates.

**State:** save v4 adds `unlocked: string[]`, the place ids paid for. v3 saves migrate with it empty.

**What it means here:**
- A crag can carry a one-time cost and a per-trip cost, both data on the place. This repo's travel and economy can take both as they are.
- The spire's sport lines need a belayer, and nobody's schedule brings one to Moonstone yet. The 2D game will need road-trip partners for them. That's a known gap, not a rule.
- Highballs (pad and spotter decisions) come in 10.3b and get their own entry.
- No new seed streams. No change to `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — Phase 10.2: the sun crosses the crag a line at a time

**Built in the 2D rebuild's Phase 10.2** (`dirtbag/app/src/sim/weather.ts`, `sunOn`; the sun's path in `content/places.ts`; the dial `CLIMB.sunSweep` in `dials.ts`; the staging in `src/view/sun.ts`). The rule is *[proposed]*: Evan hasn't ruled on it. The source of truth is the dirtbag repo's `docs/ROADMAP.md`, Phase 10, "As built (10.2)".

**What changed in the rules:**
- **Before,** every line at a sunny crag greased at the same minute (`greaseFrom`): 3 PM on a prime day, 2 on a fair one, noon on a hot one.
- **Now** the sun crosses the crag in `CLIMB.sunSweep` minutes (120), and each line greases when it arrives:
  - `sunOn = greaseFrom + sunSweep × i / (n − 1)`, rounded to the minute;
  - `i` is the line's place in the crag's sun path of `n` lines.
- **`greaseFrom` changes meaning.** It is now the sun's first minute on the wall, half a sweep before the old single time: 2 PM prime, 1 PM fair, 11 AM hot. The path's middle line keeps the old time, so the average doesn't move.
- **Roadside's path** runs from the boulder field's far end to the road:
  - rsopen, project, highball, fingercrack, testpiece, pump, crimpfest, roadside, dyno, warmup, warm;
  - projects lose their shade first, and warm-ups keep it longest.
- **Unchanged:** shaded crags (the Gorge) never get the sun, and a sunny crag without a path greases all at once.
- **The go's once-a-day line** is now "Sun's on this line now. Everything feels greasy."

**Staging** (2D only):
- The scene's sun edge crosses the crag and passes each line's foot at its sun time.
- Wet rock shows after rain.
- The map tags closed and soaked crags.
- The beta sheet says when a line gets the sun.

**What the 2D harness measured** (12 seeds × 28 days):
- All four season targets pass.
- The first V5 go comes at day 19 for every strategy.
- It's balance-neutral within noise. The bots don't plan around the shade, so the gain goes to a player who does.

**What it means here:**
- A crag's sun is data: an ordered list of route ids, and one dial.
- A 3D crag can drive its light from the same numbers: a terminator that crosses each line's base at `sunOn`. In a watched session, the shade line is the prime window made visible, as the 2D game has it.
- No new seed streams. Nothing here touches `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — Phase 10.1: the board at Send City

**Built in the 2D rebuild's Phase 10.1** (`dirtbag/app/src/sim/content/gym.ts`; the board's painter in `src/view/paint/gym.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, Phase 10, "On the rebuild".

**The board** (v0.956's "persistent gym walls"):
- Send City gains a steep board beside the weekly wall. It holds four problems, V4, V5, V6 and V7, left to right.
- They stay up four weeks (`BOARD_WEEKS`), then all four reset together. The block is `floor((day - 1) / 28) + 1`, so a block runs days 1–28, 29–56, and so on.
- Each problem is a library boulder: one crux, two beta, 6–9 moves, 12 ft. Styles lean steep: each problem draws from power, crimp, power, crimp, dyno, technical (power and crimp twice as likely).
- Ids are `bd-{block}-{n}`. An old id still resolves, so a route log keeps its name after the reset.
- The gym's lines are the week's six, then the board's four. The wall's rules apply unchanged: a day pass, and closed from 22:00.
- No change to the state's shape: board problems are logged by id like any other line.

**New seed stream:** `worldgen`, derived as `sendcity-board-{block}`. Per problem, in order, it draws:
1. the style: `int(0, 5)` into the list above;
2. the name: `int` into that style's unused names (or, once they run out, any unused board name);
3. the moves: `int(6, 9)`;
4. the crux's start: `moves × float(0.4, 0.6)`, rounded to 2 places;
5. the crux's length: `float(1.4, 2)`, rounded to 2 places.

**What the 2D harness measured** (12 seeds × 28 days × every start and strategy):
- **Before the board**, 4–10 of every 12 runs hit a day with nothing new to try, from about day 25. That day was always wet, with the week's set done.
- **With it**, 3 runs of 144 do, on days 26–27. In each, the climber had already sent all four board problems.
- The median climber ends the month at V4, not V3: the board fills wet days with hard climbing.
- The first V5 go comes about a day later (days 19–20, from 18–18.5): bots work the board's V4 first.
- All four of R2's season targets still pass.
- Board grades climb like the wall's, not stiffer:
  - board V4s and wall V4s are both sent at a median climber grade of V3;
  - 3 runs sent the V7 at V4.
  - Real boards run stiff; whether this one should is open *[proposed: Evan's call]*.

**What it means here:**
- The board is content: a set generator, like the weekly wall's. This repo can take it as a DataTable plus a generator on the `worldgen` stream.
- The derivation label above is the whole contract. Match it, and a seed sets the same board in both games.
- No change to `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — R2's end: Act I, pace on the wall, the send card, and the season's targets

**Built in the 2D rebuild's R2** (`dirtbag/app/src/sim`: `content/story.ts`, `story.ts`, `climb.ts`, `harness.ts`; the card in `src/view/paint/card.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R2 "Plan", whose *As built* notes mark the deviations and the calls still *[proposed]*.

**Act I, the demo's goal ladder** (v0.956's "Your First Season" quest, in its words):
- Five stages, in order:
  1. have $60 in hand;
  2. send 3 lines anywhere (+1 technique);
  3. send 2 outside;
  4. be regulars with someone (v0.956 wanted 15 reputation, which the rebuild doesn't have);
  5. climb V4 (+2 head).
- Stages are checked after every action, in order, and several can complete at once.
- The act ends with v0.956's closing text and $40. Dex's race starts the same night.

**Pace** *[proposed]*: a go climbs at the base rate × `clamp(1 + 0.08 × margin, 0.8, 1.25)`.
- The margin is your level in the route's style against its grade, fixed at the tie-in with the other mods.
- Fewer seconds on the wall means less pump for the same moves.
- v0.956 had no climb speed. The 2D harness shows no measurable change to the season.

**On the wall** (Phase 9). This is staging, not rules:
- the fall sheet draws the go as a bar against your best before it;
- a chalk band follows what the go climbed, and an X marks where it came off;
- past 55 pump the climber shakes, and past 45 the screen's edges close in.

**The send card** (2D only): a first send offers a PNG drawn on the device from the line's topo. It carries the name, grade, how it went ("Redpoint, go 6"), the crag, the day and season, the climber, and any first-ascent credit. Nothing is uploaded.

**The first season, as the 2D harness measures it** (12 seeds × 28 days × every start and strategy):
- A balanced climber's runway is 2.3–3.4 days at days 7–28, and no run is ever stuck.
- First-month injuries: 0% for moderate bots, 3% for reckless ones.
- An hour on the rock teaches 5.8–12.4 skill points (V0–V4); a setting shift, 0.75.
- The first V5 go comes around day 18.

**What it means here:**
- Act I is data: stages with an aim, a text and rewards. This repo's onboarding can take the same ladder as a DataTable.
- Pace matters to the sim only if this repo adopts the 2D attempt model, which is still undecided (see the R1 entry). If it does, the speed mod joins the pump factor at the tie-in.
- In a watched 3D session, pace is also staging: a stronger climber visibly moves faster. That is one answer to Phase 9's "two climbers play the same route differently". The 3D wall needs its own answer to "how close was that go".
- The season numbers above are what this repo's sim should reproduce if it ports the 2D rules.
- No new seed streams. Nothing here touches `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — R2 people: bonds, Sage's arc, Dex and the race, Scout

**Built in the 2D rebuild's R2** (`dirtbag/app/src/sim`: `presence.ts`, `curves.ts`, `content/people.ts`, `content/dog.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R2 "Plan", whose *As built* notes mark the deviations from v0.956.

**Bonds.**
- v0.956's tiers: Stranger 0, Acquaintance 1, Regular 3, Partner 5, Ride-or-Die 7.
- A day counts once toward the bond: climbing where a partner you've met is, or their belaying you. This is Phase 6's cap on v0.956's +1 per go.
- Each tier adds 0.07 to a partner's daily odds of turning up. Sage's base is 0.55, so Ride-or-Die Sage turns up five days in six.
- A Regular comes to the Gorge if asked before 1 PM on a dry day, and belays there.

**Arcs.**
- v0.956's four beats, at bonds 1, 3, 5 and 7, at least 5 days apart. The first beat waits a day past meeting.
- Sage's second beat sends her guiding for 7 days (v0.956: 16).
- The last beat needs a crag.

**Grades on curves of their own** (Phase 6: no rubber band):
- Sage's level is 4.3 + day/20, capped at 13 (v0.956's rate for her).
- Dex's level:
  - It is `min(peak, 5 + 0.045 × days trained) + 0.4 × form`.
  - The peak is 9–11.
  - He trains every day except a hurt stretch of 14–24 days that starts on a day from 35 to 75.
  - Form is a per-week value in [−1, 1], eased between weeks.

**Dex.**
- He appears where you send your first V4 (any V4+), for the rest of that day.
- After that he's out 2 days in 5, from 10:00 to 17:00: the crag on 70% of dry days, else the gym; never while hurt.
- The race: the night you're climbing V4 (displayed), if the open project is unclaimed and unsent, you get 5 days.
  - Send it and the first ascent is yours.
  - Otherwise he takes the first ascent, with a name from v0.956's list of 14.

**Scout.**
- He picks you on your 10th crag trip.
- Kibble costs $6, fills his bowl and adds 3 bond (on a 0–100 scale).
- Play is 60 minutes, once a day, for 10 bond. Each drive adds 2.
- Each night takes 22 food.

**What it means here:**
- **The seed means more again.** New derived streams, all on `events`:
  - `dex`: peak, hurt start, hurt length, in that order;
  - `dex-form-{week}`;
  - `dex-{day}`: out, then crag.
  - Sage's `sage-{day}` draw is unchanged. Only its threshold now reads the bond.
  - Picks that hold for a day hash `"{seed}:{key}:{day}"` with FNV-1a; Dex's first-ascent name hashes `"{seed}:{route}"`.
  - If the Unreal sim schedules people, draw them the same way, or a seed stops meaning the same thing in both games.
- Presence now reads state (bond, time away, invites) as well as the seed and the clock. It's still a pure function of both, so nothing about tomorrow is stored.
- None of this touches `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — R2 so far: windows close faster above your grade; load and injury; sandbags, first ascents and the Gorge

**Built in the 2D rebuild's R2** (`dirtbag/app/src/sim`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R2 "Plan"; deviations from v0.956 are marked there.

**The window curve changes (replaces R1's formula below).** R1 scaled a beta's window by clamp(1 + 0.24·margin, 0.4, 1.4). The R2 harness showed the 0.4 floor let a V4 send the Gorge's V9: past two and a half grades over, every line was equally hard, and patient goes got there. Now:
- margin ≥ 0: min(1.4, 1 + 0.24·margin), as before;
- margin < 0: max(0.002, e^(0.9·margin)), about 40% of the window per grade over.
- With the harness's human-ish hands (throws and taps scattered about 50 ms, tension tracked with a lag of about 200 ± 70 ms), a one-crux boulder goes about 60% a go one grade over, 15% at two, 5% at three and never at four. A two-crux sport line, with pump, goes 10–30% a go one grade over.
- v0.956 made the same point with a hidden cliff: odds capped at 13% 1.5 grades short and 3% at 2.5. This keeps the slope without the step.

**Load and injury** (v0.956's acute:chronic model, with Phase 6's fixes):
- Each go's load = its energy cost × (1 + grade/9) × (0.6 + 0.4 × share of the line climbed). At sleep, acute and chronic load move toward the day's total (α 1/4 and 1/14). Both start at 20, so a first week can't spike.
- The ratio is read as if you slept now: today's goes count straight away, and a morning with no goes reads like a rest day.
- Over 1.3, each go risks an injury: 0.3 per 0.1 of ratio (capped at 40%) × style (crimp 1.5, crack 1.3, power and dyno 1.2) × 1.6 if cold × hunger × the go's size against a v0.956 go. Over 1.5, skill gains ×0.6. Over 1.7 you can't climb.
- Cold: a line at or above your grade before your first go of the day. Its windows are ×0.9.
- Injuries last 2–4, 6–9 or 13–18 days by tier. The clinic copay is $0, $45 or $210, and your first injury is free.

**Sandbags and first ascents:**
- A line can climb at a grade other than its label (`trueGrade`). Windows, pump and skill gains all use the true grade, and your first go on it says so.
- Open lines have no ascent. Send one first and you name it, and call its grade soft (−1), true or stout (+1), as in v0.956. From then on, the line goes by your name and your grade.

**Granite Gorge.** It opens at V4, closes in spring, and is shaded: no afternoon grease, and heat doesn't narrow the windows. Six boulders from V5 to an open V9 (Gorge Dyno is a V7 that climbs as V8) and three sport lines.

**What it means here:**
- The curve only matters if this repo adopts the 2D attempt model, which is still undecided (see the R1 entry). If it does, adopt this curve, not R1's.
- Load is a pure sim rule and ports as it stands. `SessionDials` would carry its numbers.
- **The seed means more again:** injury rolls draw from `session` → `injury-{day}-{go n}`. If the Unreal sim rolls injuries, draw them the same way.

---

## 2026-09-28 — R1: the climber's skills set the windows; the week's rules

**Built in the 2D rebuild's R1** (`dirtbag/app/src/sim`), ported from v0.956 where it had an answer. The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R1 "Plan"; deviations from v0.956 are marked there.

**The resolved attempt model (settles the open difference in the entry below).** In 2D there is still no dice roll. Your skills set how wide each crux's window is; the verb's execution decides whether you're through.
- v0.956's five skills and grade curve: grade = floor((−8 + √(64 + 9.6·average)) / 4.8). The four starts are v0.956's archetypes.
- Each beta has a style (crimp, power, endurance, technical, dyno, crack). The style leans 65/35 on two skills, as in v0.956.
- margin = level(mix) − route grade, in grades. A beta's window = its base width × clamp(1 + 0.24·margin, 0.4, 1.4) × the day (prime rock 1.1, hot 0.9, seeping 0.92, sun on the wall 0.8, hunger down to 0.7).
- Pump rate × clamp(1 − 0.12·margin(endurance mix), 0.6, 1.5).
- The window scale is fixed at the tie-in. Pump and thin skin still narrow a window as the go goes on.
- Skill growth: v0.956's formula, but the route's style takes 60% and the beta you tried takes 40%. Goes are shorter than v0.956's, so ×0.6, and a lap of a sent line teaches ×0.3 (standing in for v0.956's staleness).

**What it means here:** `ResolveAttempt` rolls odds, with execution as one input. The 2D model has no roll: skills scale the window and execution is the whole outcome. Adopting it here would change `ResolveAttempt`, so it moves the golden vectors: save-breaking by this repo's rules. **Not decided here.** It needs a logged decision when this repo reaches its attempt work. Until then, `ResolveAttempt` and its vectors stay as they are.

**The week's rules, now spec:**
- **Money:**
  - What you can't cover goes on a card with a $150 limit. Past it, purchases are declined and the van spot becomes a cold night in the pullout. The van runs on fumes, so you're never stranded.
  - Bills ($70) land every seventh night regardless. Work needs energy but never food.
  - No game over: v0.956's hard wipe is gone.
- **Body:**
  - Fed (v0.956's hunger), −15 a night. Under 35, windows shrink; at 0 you can't climb.
  - Energy and skin come back with sleep.
- **Weather:**
  - One sky a day from the seed, with v0.956's 14-day seasons and weights (fall first).
  - Rain closes the crag, and it seeps the day after. Prime friction; hot days grease early.
- **People keep hours**, from the seed, the day and the clock:
  - Hazel belays at the crag from 8 to 5 on dry days.
  - Sage turns up about half the days, and always at the gym on day two.
  - A rope needs a belayer who's there.
  - Watching Sage is the third way to earn beta, after falling and asking.
- **Send City** resets six problems (V0–V5) each week from the seed. A problem's id carries its week.

**The seed now means more.** These derived streams are part of what a seed is, like the golden vectors:
- `worldgen` → `sky-{day}` and `sendcity-w{week}`;
- `events` → `sage-{day}`.

If the Unreal sim draws weather or schedules, draw them the same way, or a seed stops meaning the same thing in both games.

---

## 2026-09-28 — The 2D game is being rebuilt: beta then send, scrappy survival, side-view scenes

**Decided by Evan** after three look-book rounds and a playable feel slice. The source of truth is the dirtbag repo's `docs/ROADMAP.md`, section "Direction and the rebuild". The 2D game is being rebuilt in a new codebase (`dirtbag/app/`); v0.956 stays live until the rebuild's first season replaces it.

**What changed in the spec:**
1. **Climbing: beta, then send.**
   - Before a go, you choose beta for each crux. The beta sets that crux's verb and window:
     - a tension hold: hold and release to keep a marker in a moving band;
     - a timing tap: tap twice on the beat;
     - hold-to-load: charge, then let go in the band.
   - HOLD TO CLIMB stays the backbone between cruxes, with pump, and rests pay pump back.
   - Better beta is earned: by falling at a crux, by watching someone, or by asking the right person.
   - Pump, afternoon sun and thin skin narrow every window.
2. **Tone: scrappy survival with a sport core.** Money stays tight all season, not just in week 1. The feel slice starts you at $41, and a crag day without a shift ends in debt at bedtime. The attempt is the star.
3. **The clock moves only on actions and travel**, never while the player stands still.
4. **2D only, and not carried here:**
   - Side-view scenes joined by a valley map.
   - The look: park-poster landscapes with comic people, drawn in code.

**What it means here:**
- **The session model survives.** "2D minigames, 3D staging" still holds, and so does `ResolveAttempt`'s contract: per-move execution in [0, 1], and the sim decides. A crux's verb result maps onto that move's `execution`.
- **What's new for the sim:**
  - Beta becomes per-crux and discrete: a chosen sequence per crux, with its own verb and window. That replaces the single `beta` scalar in `AttemptInput`.
  - Unlocking beta is a sim rule: falls per crux, and what people know.
- **A difference still to settle.** In the 2D rebuild so far, the verb alone decides a crux; there is no dice roll. `ResolveAttempt` rolls odds, with execution as one input.
  - The 2D rebuild's next milestone (R1) adds the climber's stats to the windows. When it does, record the resolved model here.
  - Until then, don't change `ResolveAttempt`: its golden vectors are frozen.
- **The RNG is shared.** The rebuild's sim uses this repo's algorithm (mulberry32, FNV-1a over UTF-16 code units, named streams) and checks the same golden vectors, so a seed means the same thing in both games.
- **No change to Phase 0.** Its question — is a minigame-driven session tense and legible on a 3D wall? — still stands. The 2D rebuild will answer the 2D half of it first.

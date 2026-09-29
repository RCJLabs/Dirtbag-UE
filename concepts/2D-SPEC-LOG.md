# 2D spec log

The 2D game is this project's spec (see `CLAUDE.md`). When the spec changes, the change is logged here with what it means for the Unreal version, so this repo never follows a spec it hasn't read. Newest first.

---

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

# 2D spec log

The 2D game is this project's spec (see `CLAUDE.md`). When the spec changes, the change is logged here with what it means for the Unreal version, so this repo never follows a spec it hasn't read. Newest first.

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

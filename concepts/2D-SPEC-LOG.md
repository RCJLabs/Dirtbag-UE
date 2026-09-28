# 2D spec log

The 2D game is this project's spec (see `CLAUDE.md`). When the spec changes, the change is logged here with what it means for the Unreal version, so this repo never follows a spec it hasn't read. Newest first.

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

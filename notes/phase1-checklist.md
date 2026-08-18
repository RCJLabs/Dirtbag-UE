# Phase 1 — back at the desk (Evan)

Everything below is editor work. The sim side of Phase 1 is done and green
(7,631 checks); nothing here needs new C++ unless something breaks.

---

## 0. What changed while you were away

Read this first or step 3 will confuse you.

- **A game instance now owns your career.** One seed, one player, one day,
  living above the level so it survives level loads. It loads the save on
  boot and writes it when you sleep.
- **Walls no longer carry their own route.** Each wall now carries gym-board
  problem **#Board Index**. Your existing wall (Board Index 0) will show up
  as *Jug Haul V0*, not the V4 you left it on — that is correct behaviour,
  not a bug. Set the index to choose the problem.
- **The wall's Climber Stats field is now a fallback**, used only in maps
  with no game instance. The real climber comes from the career.
- **The grade ladder was recalibrated and everything got harder.** A default
  50-stat climber is now a *V5 climber*: V4 ≈ 85% first go, V5 ≈ 23%, V6 ≈
  1%, V7 ≈ 0%. If a V6 feels impossible now, that is the point.
- **A session degrades across its length.** Energy costs scale with how far
  the line is above you, and low energy costs you nerve. Burn twelve is
  meaningfully worse than burn two.

Gym board, in index order: 0 Jug Haul V0 · 1 Slab of Regret V1 · 2 Pink
Crimps V2 · 3 Dyno Tax V3 · 4 Setter's Revenge V4 · 5 Volume Country V5 ·
6 Campus Special V6 · 7 The Blue One V7. Roughly one in six carries a
hidden sandbag.

---

## 1. Get the build green (gate for everything else)

1. **Close Unreal and Visual Studio completely.**
2. **GitHub Desktop → Fetch origin → Pull.** Confirm the latest commit is
   *"Save v2: ledgers record the grade, and the first real migration"*.
3. **Delete** `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Binaries`.
4. **Double-click** `DirtbagUE.uproject` → **Yes** to rebuild. Two to five
   minutes.

**If it fails:** open
`C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Saved\Logs\DirtbagUE.log`, scroll to
the bottom, and send the lines containing `error` (ten lines around the
first one is plenty). The fix that should have solved the last failure was
two missing bridge files; the only engine code touched since is three
mechanical lines in a struct converter.

---

## 2. Verify the plumbing (two minutes, do not skip)

1. **Golden vector.** Press Play, open **Window → Output Log**, filter
   `golden`. Five numbers starting `0.59432327281683683`. Unchanged = the
   sim is still bit-exact in the engine.
2. **New nodes.** Any Blueprint → right-click → search `dirtbag`. You should
   now see **Dirtbag|Day** (Wake Up, Eat Meal, Work Shift, Sleep To Next
   Day, Gym Board, Day Attempt…) and **Dirtbag|Save** (Save To File, Load
   From File). If those categories are missing, the module didn't rebuild —
   back to step 1.

---

## 3. Point your walls at the board

1. Select your existing wall actor. In **Details → Dirtbag|Route**, set
   **Board Index** to `4` (Setter's Revenge V4 — comfortable for a starting
   climber).
2. **Ctrl+W** to duplicate it three times. Drag each copy along the wall (or
   build more slabs) and set their Board Indexes to `2`, `5`, and `7`.
   That gives you a V2 warmup, a V4, a V5 at your limit, and a V7 that
   should refuse you — the spread the Phase 1 gate wants.
3. **Space them out.** Each wall listens for **E** while you're inside its
   approach trigger; overlapping triggers mean one press hits two walls.
   A few metres of clearance between them is enough.
4. Each duplicate keeps its anims, mesh and spline, but the spline points
   still sit where the original's did — drag each copy's **HoldLine** points
   onto its own slab.

---

## 4. Place the sleep spot

1. **Place Actors** panel → search `Dirtbag Sleep Spot` → drag it into the
   level, well away from the walls (that corner is "the van" until the van
   exists).
2. It has **no visible mesh** — drop a cube next to it so you can find it,
   or note where it is.
3. **Ctrl+S** to save the level.

---

## 5. Play the loop

1. Press Play. Walk to a wall → the prompt names the board problem and its
   grade. **E** to start, **hold Space** to load a move, release in the
   green window to commit.
2. Watch the new **DAY / $ / ENERGY** row: energy now drops per burn, faster
   on hard lines.
3. Climb the V2, then the V7. **They should feel like different sports.**
4. Walk to the sleep spot → **E** → *"Day 2. $xxx. Skin x.x. Saved."*
5. Climb again on day 2: skin carried over minus the night's regrowth,
   warmth reset to cold, and beta on yesterday's projects still there.

---

## 6. The save test (Phase 1's third gate)

1. After sleeping at least once, **check the file actually exists**:
   `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Saved\SaveGames\dirtbag-save.txt`
   Open it in Notepad — it is human-readable on purpose, starts `version=2`,
   and lists every route you've touched with its grade and attempt count.
   **If the file is missing, tell me** — that means the write is failing
   silently and the "Saved." message is lying.
2. **Stop PIE completely. Press Play again.** You should resume on the day
   you slept into, with cash, skin, skills and every route's attempt history
   intact.
3. Delete the file to start a fresh career.

---

## 7. What to send back

In rough priority:

1. **Did it build?** If not, the error lines.
2. **Did the save survive a PIE restart?** That's a Phase 1 gate.
3. **Does the V2 feel different from the V7?** That's the other gate — and
   the one only you can answer.
4. **Feel notes on the new fatigue**: does a long session actually degrade
   in a way you can feel, or is it invisible? Every complaint maps to a
   named dial (`Dirtbag|Verb` and `Dirtbag|Staging` on the actor for feel,
   `DayDials` in the sim for consequence).

Once the build is confirmed green, the next container-side bite is ready to
go: a **Read Route** node so a wall's prompt says *"Pink Crimps V2 — this
should go"* instead of a bare name, a career readout node (*"You climb V5.
Campus Special has taken 14 burns and counting."*), and eat/work interact
spots so the day's other verbs have somewhere to happen.

---

## 8. After the next pull — eat, work, and the ambient HUD

New since the checklist above. Rebuild first (delete `Binaries`, reopen).

1. **The HUD is automatic.** No placement: the game mode now draws
   `DAY 3 · 09:42 · $412 · energy 78 · hunger 21 · skin 6.4` plus your
   career line, always, not just mid-session.
2. **Place two more spots.** Place Actors → `Dirtbag Day Spot` → drop one
   near the wall and set **Kind = Meal** (the cooler), another anywhere and
   set **Kind = Shift** (the front desk). Same walk-up-and-E as sleeping.
   Keep every spot's trigger clear of the walls' triggers.
3. **Your existing sleep spot keeps working** — it is now a Day Spot with
   Kind pinned to Sleep. If it misbehaves after the rebuild, delete it and
   place a fresh `Dirtbag Day Spot` with Kind = Sleep; 30 seconds.
4. **Wall prompts now read the line**: *"Pink Crimps V2 — this should go"*
   / *"The Blue One V7 — not this year."* Judged against the guidebook
   grade, so a sandbagged line will lie to you exactly as it should.

**Now a full day is playable:** wake → read the board → climb → eat when
hunger climbs → take a shift when cash gets thin → sleep, and the career
line at the bottom of the screen grows a nemesis. That is the Phase 1
day-loop gate, minus the van.

# Phase 0 blockout — editor checklist (Evan)

The goal of this pass, in one line: a figure travels the wall spline
hold-to-hold, hesitates where the odds were thin, and falls where the
timeline says — with zero art. Legibility before beauty; that is the whole
Phase 0 question.

Container-side prerequisites are done: sim, Blueprint nodes (search
"Dirtbag" in any graph), golden vector verified in-editor, BP_SimTest
proven deterministic. Everything below is editor craft.

## A. Animations in (SETUP.md §4 step 0)

1. Add the Hammerhead climbing pack to the project (Fab plugin in-editor, or
   the launcher's "Add to project").
2. If its skeleton isn't the UE5 Manny/Quinn: select its anim sequences →
   right-click → **Retarget Animations** → target the template's skeleton.
3. Make a working folder `Content/Dirtbag/Anims` and copy/rename the four
   Phase 0 needs so later steps don't care about pack naming:
   - `AS_HangIdle` (hanging on the wall, idle)
   - `AS_ReachL`, `AS_ReachR` (a move to the next hold, each side)
   - `AS_Fall` (peel-off)
   Anything else in the pack is a bonus, not needed yet.

## B. Room + wall + spline (§4 step 1)

1. Duplicate the ThirdPerson map (walk-up character and GameMode come for
   free): right-click `Lvl_ThirdPerson` → Duplicate → `Lvl_GymBlockout`
   under `Content/Dirtbag/Maps`. Open it; clear clutter later, not now.
2. The wall: place a Cube actor, scale ≈ X=6m, Y=0.3m, Z=5m, pitch it
   ~15° past vertical (a slight overhang reads "climbing wall" instantly).
   Gray material. Add a thin cube at the base as the mat.
3. New Blueprint actor `BP_ClimbWall` (in `Content/Dirtbag/BP`):
   - Add a **Spline** component.
   - Place the actor at the wall base; in the level, alt-drag spline points
     up the wall face — **8 points**, bottom to top, wandering a little
     left/right like a real line. Keep points a forearm's length off the
     wall surface so a mesh at a point doesn't clip.
   - Optional but worth it: in the construction script, add a small sphere
     static mesh at each spline point — instant hold markers.

## C. Walk-up and session frame (§4 step 2)

1. In `BP_ClimbWall`: add a **Box Collision** at the base of the wall and a
   **Camera** component placed ~4m back and ~2m up, framing the whole wall
   at a slight angle (the "watched session" frame).
2. On box **BeginOverlap** (other actor = the player character): Print
   "Press E to climb" for now (a real prompt widget is polish).
3. Add an **E** key event (enable input on overlap: Enable Input / Disable
   Input on end overlap). On E:
   - Get Player Controller → **Set View Target with Blend** → this actor,
     blend 0.5s. That one node IS the session frame.
   - Hide the third-person character (Set Actor Hidden, disable input on
     it) — the climber figure is separate, next section.

## D. The staged replay (§4 step 3) — the meat

Cast: a **Skeletal Mesh** component on `BP_ClimbWall` (the template's
Quinn/Manny mesh), hidden until the session starts, playing `AS_HangIdle`,
positioned by spline point index. It is a puppet, not a Character — no
movement component, you place it with Set World Location.

Session flow on E (after the camera blend):

1. Once per level: **Build Route** — World Seed `gym-1`, Route Name
   `First Blood`, Grade/TrueGrade 4, Power, Boulder. **Start Session** with
   a Make DirtbagClimber (defaults) into a `Session` variable; add a
   `Memory` (Dirtbag Project Memory) variable.
2. **Attempt In Session** (seed `gym-1`, Session, Memory, climber, route) →
   store the result. Show the puppet at spline point 0, `AS_HangIdle`.
3. Walk the result's **Timeline** array with an index variable and a
   looping structure driven by **Set Timer by Event** (retriggerable) —
   do NOT use a ForEach with Delay inside (delays don't work in loops):
   - Per element: wait `0.4 + (1 - Odds) * 1.5` seconds at the current
     hold (that pause IS the hesitation read — a 95% move flows, a 40%
     move visibly gathers itself), then:
   - If **bSuccess**: play `AS_ReachL`/`AS_ReachR` (alternate), move the
     puppet to the next spline point over ~0.6s (VInterp on Tick, or a
     Blueprint Timeline float — either is fine at blockout quality), back
     to `AS_HangIdle`, advance the index.
   - If **not bSuccess**: play `AS_Fall`, lerp the puppet down to the mat,
     stop the walk.
4. End of timeline with **bSent** true: move over the lip, Print the style
   (Break the result → Style) and `Grade Name`. Fell: Print highpoint like
   "fell at move 5 of 7".
5. After either ending: 2s pause, Set View Target back to the player,
   unhide the character, re-enable input. Attempt again on the next E —
   Session and Memory are already updating in place, so warmth/skin/beta
   carry across burns for free. Watch repeated attempts on a Grade 7
   sandbag: the highpoint creeping up across burns is the projecting loop,
   visible.

**Done for this checklist** when: a V4 replay reads as a cruise, a V7
replay (change two numbers on Build Route) reads as a fight that ends on
the mat, and someone watching can tell which is which without being told.
That's Done-when #1 in embryo. Pump bar and the interactive HOLD TO CLIMB
verb (§4 steps 4–5) come after this works — the live-attempt nodes are
already waiting.

Push the project when the replay works (or when you stop for the day —
half-done is fine, it's a branch). If any of this fights you — retarget
mess, spline weirdness, timer logic — say so specifically; the fix might
be me writing a C++ staging actor you just drop in, which is on the table
if Blueprint scripting is the bottleneck.

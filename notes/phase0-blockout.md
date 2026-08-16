# Phase 0 blockout — editor checklist (Evan)

The goal, one line: a figure travels the wall spline hold-to-hold,
hesitates where the odds were thin, and falls where the timeline says —
zero art. Legibility before beauty; that is the whole Phase 0 question.

The heavy lifting is now C++: `ADirtbagClimbWall`
(`Source/DirtbagUE/DirtbagClimbWall.*`) handles the trigger, the E prompt,
the camera blend, the sim call, and the whole staged replay. The editor
work left is placement and assignment — no Blueprint scripting required.

## A. Animations (pack already added)

1. Find the pack's content folder in the Content Browser. Play sequences
   until you've identified: a hanging idle, a reach/climb move (one per
   side if it has them, otherwise one is fine for both slots), and a fall.
2. If the pack's skeleton is not the template's (drag an anim onto the
   level's Quinn/Manny to check — if it refuses or T-poses, it isn't):
   select its anim sequences → right-click → **Retarget Animations** →
   target the template skeleton. Use the retargeted copies from here on.
3. No renaming needed — the actor takes anims via dropdowns.

## B. Map and wall

1. Right-click `Lvl_ThirdPerson` → **Duplicate** → name it
   `Lvl_GymBlockout` (put it in a `Content/Dirtbag/Maps` folder). Open it.
2. Place a **Cube** (Quick Add menu → Shapes → Cube). In its Details:
   Scale ≈ (6, 0.3, 5) → a 6m-wide, 5m-tall, 30cm-thick slab. Rotate it
   ~15° so the top leans toward the player side (slight overhang). Drop a
   second cube, scale (7, 3, 0.1), flat at the base — the mat.

## C. Place the wall actor

1. Compile first (pull + open project → it rebuilds; or Ctrl+Alt+F11).
2. In the Place Actors panel (Window → Place Actors), search
   `Dirtbag Climb Wall` and drag it into the level. Put its origin at the
   **base of the wall, on the mat** — fall height and the trigger position
   key off it. Rotate it so its Y axis points away from the wall (the
   default camera and trigger sit on +Y).
3. With the actor selected, in the Details panel:
   - **Climber** component → Skeletal Mesh → pick the template's
     `SKM_Quinn` (or Manny).
   - **Dirtbag|Anims** → assign Hang Idle / Reach Left / Reach Right /
     Fall from the pack (Reach L and R can be the same clip).
   - Leave **Dirtbag|Route** at defaults (`gym-1`, "First Blood", V4).
4. Select the actor's **HoldLine** (spline) component. In the viewport,
   grab the spline's end points; right-click a point → **Duplicate Spline
   Point** to add more. Drag ~8 points up the wall face, bottom to top,
   wandering a little left/right like a real line. White marker spheres
   appear at each point automatically — keep them a forearm off the wall
   so the mesh doesn't clip.
5. Check the camera: select the **SessionCamera** component — a preview
   window shows its view. Nudge it until the whole wall is in frame.

## D. Run it

PIE → walk to the wall → prompt appears → **E**. The character hides, the
camera blends to the session frame, the puppet climbs the spline with
sim-driven hesitation, and either tops out (style card) or peels to the
mat ("Off at move N"). E again for the next burn — warmth, skin, and beta
carry across attempts automatically, so a hard route's highpoint creeps
up over burns: that is the projecting loop, visible.

**Done when:** at Grade 4 it reads as a cruise; set Grade/TrueGrade to 7
on the actor and it reads as a fight that ends on the mat — and a watcher
can tell which is which without being told. Then push the project.

Tuning lives on the actor under **Dirtbag|Staging** (hesitation, move
speed, fall speed, end pause) — feel free to turn those; they are
presentation dials, not sim dials. Next after this works: the pump bar and
the interactive HOLD TO CLIMB verb (the live-attempt nodes are ready).

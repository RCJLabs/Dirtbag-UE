# The naming widget — click by click

Everything else in the first-ascent pipeline is C++ and needs no editor
work. **Cleaning is already bound: walk up to a line and press C.** The only
thing that genuinely needs building is the box you type a name into, because
typing needs UMG and UMG is not something I can make from here.

Budget about 20 minutes. There is no C++ to write and nothing to rebuild
unless you have not pulled yet.

---

## 0. Pull and check it works before building anything

1. Close Unreal. **Fetch origin → Pull.** Delete
   `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Binaries`. Reopen the
   `.uproject`, **Yes** to rebuild.
2. Select the wall and set **Details → Dirtbag|Route → Venue = `Crag`**.
   This is the switch that moves a wall outdoors; there is no global one.
   (An earlier version of this file said to set `b Indoors` on the
   DirtbagGameInstance — you cannot, because a game instance is not an actor
   and has no details panel. If you followed that and got *The Blue One V7*,
   this is why: the index clamped to the eight-problem gym board.)
3. Set the same wall's **Board Index** to `25` — *the arete left of Diesel*,
   the easiest of the three open projects (it is really V7, though the book
   only guesses).
4. Press Play, walk up to it. The prompt should read:

   > `the arete left of Diesel  V7   unclimbed — untouched: moss, dirt, and a rumour of a line`

5. **Press C four times.** Each press is 30 minutes on the brush and the
   toast should walk up through *filthy* → *workable* → *brushed*. Watch the
   clock on the HUD move — this is time you are not climbing in.
6. Try climbing it before cleaning and after. Filthy it is essentially
   impossible; clean it is merely very hard. That is the mechanic.

If all of that works, the pipeline is live and only the naming is missing.

---

## 1. Make the widget

**Content Browser** → right-click in `Content` → **User Interface** →
**Widget Blueprint** → **User Widget** → name it `WBP_NameFirstAscent`.
Double-click to open.

You are in the **Designer** tab (top right toggles Designer/Graph).

1. In the **Palette** (left), drag a **Canvas Panel** onto the graph if
   there is not one already.
2. Drag a **Vertical Box** into the canvas. In **Details** (right), find the
   **Anchors** dropdown and pick the **centre** box. Set **Position X** and
   **Position Y** to `0`, **Size X** `600`, **Size Y** `240`, and tick
   **Size To Content** off. Then set **Alignment** X `0.5`, Y `0.5` so it
   sits centred rather than hanging off the middle.
3. Drag a **Text** widget into the Vertical Box. Rename it (double-click in
   the Hierarchy) to `HeaderText`. In Details → **Content → Text**, type:
   `FIRST ASCENT`.
4. Drag a second **Text** into the Vertical Box, under the first. Name it
   `LineText`. Leave its text empty — the graph fills it in with which line
   you just did.
5. Drag an **Editable Text (Box)** into the Vertical Box. Name it
   `NameBox`. In Details → **Hint Text**, type `name it`.
   - **Tick `Is Variable`** at the top of the Details panel for this one.
     Without it the graph cannot read what you typed. (Text and Button
     widgets you named above should have `Is Variable` ticked too — it is on
     by default when you rename them, but check.)
6. Drag a **Button** into the Vertical Box. Name it `ConfirmButton`. Drag a
   **Text** *inside* the button and set its text to `Name it`.
7. Optional but nice: a second **Button** called `LaterButton` with text
   `Later`.

Layout does not matter much. Legible beats pretty at blockout.

---

## 2. Wire the widget's graph

Switch to the **Graph** tab (top right).

### Fill in which line it was

1. Find the **Event Construct** node (it is there by default; if not,
   right-click → search `Event Construct`).
2. Drag off its white execution pin → search **Cast To
   DirtbagGameInstance** → place it.
3. **Right-click on empty graph space** — do not drag from a pin — → search
   **Get Game Instance** → place it. Connect its blue return pin into the
   cast node's **Object** input.

   > **Why that order.** `Get Game Instance` is a *pure* node: it has no
   > execution pins at all. Dragging off an exec pin filters the search to
   > nodes that take one, so it is not in that list and never will be. An
   > earlier version of this file said to drag it off the exec pin, which
   > cannot work. If you see `Get Unique Instance` in the list, that is a
   > different node — not this one.
4. From the cast's **As Dirtbag Game Instance** pin, drag → **Get Naming
   Line Text**.
5. In the **Variables** list (bottom left, under this widget), drag
   `LineText` into the graph → from it drag → **Set Text (Text)**.
6. Connect: `Event Construct` → `Cast` → `Set Text`. Plug **Naming Line
   Text** into the Set Text node's **In Text** pin.

That makes the prompt say *the arete left of Diesel* rather than nothing.

### The confirm button

1. Select `ConfirmButton` in the Hierarchy. In **Details**, scroll to
   **Events** and click the green **+** next to **On Clicked**. It drops an
   `On Clicked (ConfirmButton)` node into the graph.
2. From that node's exec pin: **Cast To DirtbagGameInstance**. Then
   right-click empty space for **Get Game Instance** and feed its return
   into the cast's **Object** pin — same pure-node reason as above.
3. From the cast pin, drag → **Name First Ascent**. It has two inputs:
   - **Board Index**: from the cast pin, drag → **Get Naming Board Index**,
     and plug that in. *Do not type a number here* — the game knows which
     line it was and typing one is how you name the wrong route.
   - **Name**: drag your `NameBox` variable in → from it drag → **Get
     Text** → then → **To String (Text)** → plug into **Name**.
4. From **Name First Ascent**'s exec pin, drag → **Remove from Parent**.
   That closes the widget.

`Name First Ascent` returns a bool. You can ignore it, or branch on it to
keep the widget open if the name was empty — it refuses an empty name.

### The later button (if you made one)

`On Clicked (LaterButton)` → **Cast To DirtbagGameInstance** (fed from a
**Get Game Instance** placed on empty space) → **Dismiss Naming** →
**Remove from Parent**.

Nothing is lost by walking away: you did the first ascent, and it stays
yours to name whenever you come back.

**Compile** (top left) and **Save**.

---

## 3. Show the widget when a line goes

The game raises a flag; something has to notice it. The cheapest place is
the level blueprint.

**Blueprints** (toolbar) → **Open Level Blueprint**.

1. Right-click in the graph → **Event Tick**.
2. From Tick, drag → **Get Game Instance** → **Cast To
   DirtbagGameInstance**.
3. From the cast pin, drag → **Get b Naming Pending** → plug into a
   **Branch**.
4. You need this to fire once, not every frame. Right-click → **Do Once**,
   and put it between the Branch's **True** pin and what comes next.
5. From **Do Once**, drag → **Create Widget**. Set its **Class** to
   `WBP_NameFirstAscent`.
6. From Create Widget's return pin, drag → **Add to Viewport**.
7. Also from **Do Once**, before or after: **Get Player Controller** → **Set
   Show Mouse Cursor** = `true`, and **Set Input Mode UI Only** (target the
   player controller, In Widget Focus = your created widget). Otherwise you
   can see the box but not type in it — this is the step everyone misses.
8. Reset the **Do Once** so a second first ascent also prompts: from the
   Branch's **False** pin, drag → the **Do Once** node's **Reset** pin.

**Compile** and **Save**.

> If you would rather not use the level blueprint, the same graph works in
> any always-present actor's blueprint. The level blueprint is just the one
> that already exists.

---

## 4. Play it

1. Point a wall at **Board Index 25** and go clean it (**C**, four presses).
2. Climb it. This will take a while — it is really V7, and if you are not
   yet a V7 climber it may take many sessions. That is the point; check
   `notes/phase2-window.md` for how long it took in simulation.
3. The moment it goes, the HUD prints **FIRST ASCENT — the arete left of
   Diesel is yours to name** in gold, and your widget appears.
4. Type a name. Press **Name it**.
5. Now the payoff: **you find out what it actually was.** The book guessed
   V7. Ask the game what it really went at — the `First Ascent Line` node
   returns something like:

   > `Roadside Rites  V7  FA you`

   and if the book had it wrong, it says so: `(the book said V8)`.

6. Sleep, quit, come back. Open
   `Saved\SaveGames\dirtbag-save.txt` in Notepad and find your line — the
   name, the confirmed grade, and the FA claim are all in there.

---

## 5. What to send back

1. **Does the naming land?** That moment is the whole gate for the phase.
   It should feel earned, not administrative.
2. **Is cleaning interesting or is it a chore?** Four presses of C is the
   current shape. If it is dull, the fix is not fewer presses — it is that
   cleaning should be something you do *while* waiting for the window, and
   that means the wait needs somewhere to happen.
3. **Was the true grade a good surprise?** The three lines here are V7, V7
   and V10 against book guesses of V7, V8 and V9. If finding out feels flat,
   the drift wants to be bigger.
4. **Anything you wanted to do and couldn't.** Naming a line you did not
   first-ascend, renaming, seeing the crag's FA list — all deliberately
   absent, and all easy to add if you want them.

---

## Two things it deliberately will not do

- **Rename.** Once named, named. Nobody renames a route, including you.
- **Let you name somebody else's line.** `Can Name Line` is false for
  everything already in the book, and `Name First Ascent` refuses it even if
  you call the node directly.

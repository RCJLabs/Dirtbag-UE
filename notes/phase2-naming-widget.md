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

> **Check it landed in `Content`, not `Engine`.** If the Content Browser
> was showing engine content when you right-clicked, the asset goes to
> `/Engine/Functions/UserInterface/` — it will not ship with the game and an
> engine update can lose it. Right-click the asset → **Move To…** → put it
> under **Content**. The path shows in the tooltip when you hover it.

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
4. Drag a second **Text** into the Vertical Box, under the first. **Rename
   it to `LineText`** — double-click its name in the **Hierarchy** panel, or
   select it and press **F2**. Leave its text empty; the graph fills it in
   with which line you just did.

   > **Renaming is what makes it a variable.** A widget left at its default
   > name (`Text Block`) is not one, so it will not appear in the Variables
   > list in the Graph tab and step 2 will look like it is missing a node.
   > If you get there and cannot find `LineText`, this is why.
   >
   > `HeaderText` does not have to be a variable — its text is static and
   > nothing in the graph writes to it. Only `LineText` does.
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

     > **Not `Get Display Name`.** It is near `Get Text` in the search and
     > it compiles perfectly, but it returns the *widget object's* name —
     > so every line you ever named would be called `NameBox`. If your
     > first ascent comes out with the wrong name, this is why.
4. From **Name First Ascent**'s exec pin, drag → **Remove from Parent**.
   That closes the widget.
5. **Give input back**, or the game is unplayable from here on. Section 3
   puts the player controller into UI-only mode to let you type; nothing
   undoes it, so without this you keep a mouse cursor and cannot move.
   After **Remove from Parent**:
   - Right-click → **Get Player Controller**.
   - **Drag off its `Return Value` pin** → **Set Input Mode Game Only**
     (its Target fills in from the pin you dragged).
   - **Drag off that same `Return Value` pin again** → **Set Show Mouse
     Cursor**, box **unticked**. Wire `Set Input Mode Game Only`'s `then`
     into its execute pin.

   > `Set Show Mouse Cursor` is not a function — `bShowMouseCursor` is a
   > *variable on PlayerController*, so it only exists as a node once you
   > drag from a controller pin. Right-clicking in empty graph space in a
   > UserWidget will never offer it. (If it still doesn't appear, untick
   > **Context Sensitive** at the top of the search popup.)

   > Both buttons need this. If only Confirm has it, walking away with
   > *Later* leaves you stuck in exactly the same way.

`Name First Ascent` returns a bool. You can ignore it, or branch on it to
keep the widget open if the name was empty — it refuses an empty name.

### The later button (if you made one)

`On Clicked (LaterButton)` → **Cast To DirtbagGameInstance** (fed from a
**Get Game Instance** placed on empty space) → **Dismiss Naming** →
**Remove from Parent** → **Set Input Mode Game Only** → **Set Show Mouse
Cursor** (unticked), both targeting a **Get Player Controller**.

The last two are not optional — see step 5 above.

Nothing is lost by walking away: you did the first ascent, and it stays
yours to name whenever you come back.

**Compile** (top left) and **Save**.

---

## 3. Show the widget when a line goes

The game raises a flag (`b Naming Pending`); something has to notice it and
put the widget on screen.

### 3a. Finding somewhere to put the graph

The level blueprint is the cheapest host, but **where it lives moved between
UE 5.x versions**, so try these in order:

1. **Main level-editor toolbar** — look for a **Blueprints** dropdown (a
   blue blueprint icon, usually right of the *Add*/*Create* button) →
   **Open Level Blueprint**.
2. **Not there?** The toolbar collapses when the window is narrow. Look for
   a **»** or **⋯** overflow at the *right-hand end* of the toolbar and open
   that; the Blueprints dropdown will be inside.
3. **Still not there?** Widen the editor window and look again — collapsing
   is width-driven.

**If you cannot find it at all, do not hunt.** The graph below works
unchanged in *any* actor that is always in the level. The level blueprint is
just the one that already exists. Good alternatives, in order of preference:

- Your **player character blueprint** (the third-person template's
  `BP_ThirdPersonCharacter` or whatever yours is called) — open it, use its
  **Event Graph**, and everything below is identical.
- Any **Dirtbag Day Spot** blueprint you have made. If yours are placed as
  the raw C++ class rather than blueprints, right-click the C++ class in the
  Content Browser → **Create Blueprint class based on…**, and place that
  instead.

> **Why any of them work.** The graph only asks the game instance a question
> once a frame. It does not care who asks. The level blueprint is convenient,
> not required.

### 3b. The graph

1. Right-click in empty graph space → **Event Tick**. (In a character
   blueprint it may already be there.)
2. Right-click in empty graph space again → **Get Game Instance**. Place it
   somewhere below Tick.

   > Same pure-node rule as §2: `Get Game Instance` has **no execution
   > pins**, so it cannot be dragged off Tick's white pin and will not show
   > up if you try. Place it on its own.

3. Drag off **Event Tick's** white exec pin → **Cast To
   DirtbagGameInstance**. Connect **Get Game Instance's** blue return into
   that cast's **Object** pin.
4. From the cast's **As Dirtbag Game Instance** pin, drag → **Get b Naming
   Pending**.
5. Drag off the cast's **then** exec pin → **Branch**. Plug **b Naming
   Pending** into the Branch's **Condition**.
6. This must fire once, not sixty times a second. Right-click → **Do Once**.
   Connect the Branch's **True** pin → **Do Once's** exec input.
7. Drag off **Do Once's** *Completed* pin → **Create Widget**. On the node,
   set **Class** to `WBP_NameFirstAscent`. Its **Owning Player** can be left
   empty.
8. From **Create Widget's** **Return Value**, drag → **Add to Viewport**.
9. **The step everyone misses.** This is a *chain*, not two loose nodes —
   both of these need their execute pins wired or they never run:
   - Right-click → **Get Player Controller** (Player Index 0).
   - **Drag off its `Return Value` pin** → **Set Show Mouse Cursor**, and
     **tick the box** `true`. It is a *variable on PlayerController*, not a
     function, so it only appears in the search when you drag from a
     controller pin — right-clicking in empty space won't find it.
   - **Connect `Add to Viewport`'s `then` pin → `Set Show Mouse Cursor`'s
     execute pin.** Setting the target alone leaves the node orphaned: it
     sits there looking wired and never fires.
   - Right-click → **Set Input Mode UI Only**. Target = the same controller.
     Connect **`Set Show Mouse Cursor`'s `then` → its execute**.
   - Set its **In Widget to Focus** to **Create Widget's Return Value** —
     pull a second wire off the same pin that feeds Add to Viewport.

   Skip this and the box appears, looks fine, and you cannot type into it.

10. So a *second* first ascent also prompts: drag from the Branch's
    **False** pin → the **Do Once** node's **Reset** input pin.

**Compile** (top left) and **Save**.

### 3c. Sanity check before you play

- Is **Class** on Create Widget actually set (it is easy to leave `None`)?
- Is the Do Once **Reset** wired from Branch **False**? Without it the widget
  appears for your *first* first ascent and never again.
- Does the chain run all the way through — **Add to Viewport → Set Show
  Mouse Cursor → Set Input Mode UI Only** — with every execute pin
  connected? A node whose *target* is wired but whose *execute* is empty
  looks finished and does nothing.
- In the **widget**, do both buttons end with **Set Input Mode Game Only**
  and **Set Show Mouse Cursor** unticked? Without those you can name one
  route and then never move again.

If Create Widget's Class is `None` the graph compiles clean and nothing ever
appears — which looks exactly like the flag never being raised.

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

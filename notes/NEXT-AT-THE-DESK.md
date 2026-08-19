# When you get home — the whole list, in order

Everything outstanding, sequenced. Roughly **90 minutes** if nothing fights
you, and the first 15 are the ones that matter most.

Detail lives in the per-feature notes; this is the running order.

---

## 0. Get the build green  ·  ~10 min

1. **Close Unreal and Visual Studio completely.**
2. **GitHub Desktop → Fetch origin → Pull.** Top commit should be
   *"The odd-jobs board, the salaried job, and a trap that is not one yet"*.
3. **Delete** `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Binaries`.
4. Double-click `DirtbagUE.uproject` → **Yes** to rebuild. 2–5 minutes.

**If it fails:** `Saved\Logs\DirtbagUE.log`, scroll to the bottom, send me
the lines around the first `error`. Everything checkable without Unreal is
green — sim harness at 27,033 checks, unity build clean, definitions,
bridges, field names and declaration order all pass — so a failure is
something the checks cannot see, and the log is the fastest way to it.

5. Press Play and confirm the golden vector still logs
   `golden[0] = 0.59432327281683683` (Output Log, filter `golden`).

> **Your save will load.** It is on an older version and migrates forward —
> v1 through v6 are all tested. Shoes and van arrive new, you owe nothing.

---

## 1. The naming widget  ·  ~20 min  ·  **the important one**

This is the only thing standing between the code and Phase 2's headline
gate. The whole first-ascent pipeline is live and tested; nothing can type
a name without it.

**Follow `notes/phase2-naming-widget.md` §1–3.** In short:

1. Content Browser → **User Interface → Widget Blueprint** → `WBP_NameFirstAscent`.
2. Designer: Vertical Box centred, with `HeaderText`, `LineText`,
   `NameBox` (an **Editable Text (Box)**, tick **Is Variable**), and a
   `ConfirmButton`.
3. Graph: `Event Construct` → cast to DirtbagGameInstance → `Get Naming Line
   Text` → `Set Text` on `LineText`.
4. `On Clicked (ConfirmButton)` → cast → **`Name First Ascent`**, with
   **Board Index** wired from **`Get Naming Board Index`** — *never a typed
   number*, that is how you name the wrong route — and **Name** from
   `NameBox` → `Get Text` → `To String`.
5. Level Blueprint: `Event Tick` → cast → `Get b Naming Pending` → Branch →
   **Do Once** → `Create Widget` + `Add to Viewport`, and reset **Do Once**
   from the Branch's False pin.
6. **`Set Input Mode UI Only`** + `Set Show Mouse Cursor`. Skip this and you
   will see the box and be unable to type in it. Everyone misses it.

**Test it fast:** point a wall at **Board Index 25** (*the slab right of the
pull-off*, V3 — the easy one), `Venue = Crag`, press **C** four times to
clean, then climb it. A V5 climber takes it in about two in-game days.

---

## 2. Place four actors  ·  ~10 min

All are `Dirtbag Day Spot`. None has a mesh — drop a cube beside each if you
want to find them again. **Keep every trigger clear of the walls'.**

| Kind | Where | What it does |
|---|---|---|
| `Fire` | at the Lot | rest **with people** — rapport, and what everyone is working |
| `Dog` | by the van | feed the stray; 3–4 meals and it is yours |
| `Van` | at the van | best repair you can afford: replace → patch → bodge |
| `GearShop` | anywhere | resole $55, new $165 |

Set the **Fire** spot's *Wait For Window* ticked, same as your Rest spot.

Details: `notes/phase2-lot.md`, `notes/phase2-dog.md`,
`notes/phase3-placement.md`.

---

## 3. Check the new keys work  ·  ~5 min

At any **crag** wall:

- **C** — clean. Prompt walks *untouched → filthy → workable*.
- **B** — ask for beta. *"Margo walks you through it."* On an unclimbed line:
  *"Nobody here has done it."*
- **E** — climb, as before.

And confirm the HUD now shows: conditions line, the dog, `owe $NN` when you
are behind, and the van's state when something is wrong with it.

---

## 4. Play a proper week  ·  ~30–45 min  ·  **this is the actual goal**

Everything above is setup. This is the part only you can do, and it is what
three phases of gates are waiting on.

Play **seven days in a row** without reloading. Wake, read the forecast,
decide, sleep. Let it go wrong.

Things to deliberately do at least once:
- Wait at the **fire** for a window rather than climbing immediately.
- Take a **first ascent** and name it.
- Let the **van** get bad enough to break, and bodge it rather than paying.
- Climb a day on **shot skin** and notice whether jugs feel like the answer.

---

## 5. What to send back

In priority order. Short notes are fine — "annoying", "didn't notice",
"felt good" are all useful.

1. **Did the naming land?** That moment is Phase 2's gate. Earned, or
   administrative?
2. **Does waiting for a window feel like a decision or dead time?** The
   fire exists to answer this. If it is still dead time, people need to
   *do* more than talk, and I would rather know now.
3. **Is a breakdown drama or noise?** Two or three in year one, climbing to
   about nine a year as the van ages. If it reads as noise the fix is fewer
   and more expensive — that is a dial, not a rebuild.
4. **Does money press on you?** A year should end tight. If you never think
   about it, the whole Phase 3 gate is still open.
5. **Anything you wanted to do and couldn't.**

---

## Known and deliberate — do not report these as bugs

- **Seasons do not exist.** One temperature centre covers the year, so
  windows sit in the same part of the day forever. This is why the salaried
  job is not yet a trap (`notes/phase3-jobs.md`), and seasons are the next
  thing I would build.
- **No town.** The gear shop is a spot in a field.
- **The salaried job is not reachable in-game yet** — it is built and
  tested in the sim, with no verb wired to it, precisely because the
  measurement says it does not cost anything yet.
- **The naming widget is the only UMG in the project.** Everything else is
  the canvas HUD on purpose, at blockout quality.

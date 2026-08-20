# When you get home — the whole list, in order

Rewritten 2026-08-20 against what is actually in the build. Roughly **2
hours** if nothing fights you. The first 30 minutes are the ones that matter.

Detail lives in the per-feature notes; this is the running order.

---

## 0. Get the build green  ·  ~10 min

1. **Close Unreal and Visual Studio completely.**
2. **GitHub Desktop → Fetch origin → Pull.** Top commit should be
   *"Wire the Shaded Cave as a venue you can actually stand at"*.
3. **Delete** `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Binaries`.
4. Double-click `DirtbagUE.uproject` → **Yes** to rebuild. 2–5 minutes.

> This pull is **big** — nine new sim files and six new bridge files. If the
> build fails it will most likely be a missing `SimXxx.cpp` in the module,
> which the definition checker already verifies, or a UHT complaint about a
> USTRUCT declared after its use, which the declaration-order checker
> already verifies. Both are green here, so a failure is something neither
> can see.

> **One build already failed here and is fixed** — three legacy properties
> were `BlueprintReadWrite` in a private section, which is valid C++ and a
> hard UHT error. `tools/check-engine-defs.py` catches that class now, so
> pull before you rebuild.

**If it fails:** `Saved\Logs\DirtbagUE.log`, bottom of the file, send me the
lines around the first `error`. Everything checkable without Unreal is
green — **43,686 harness checks**, unity build clean, and four checkers
(definitions, bridges, field names + declaration order, mirror coverage) all
at zero problems.

5. Press Play and confirm the golden vector still logs
   `golden[0] = 0.59432327281683683` (Output Log, filter `golden`).

> **Your save will load.** We are on **SAVE_VERSION 13** and **v1 through
> v12 all have tests**. Everything new arrives as it truthfully was: one
> crash pad, no board, no gym membership, not injured, no training load, no
> sponsor, nobody came before you.

---

## 1. The naming widget  ·  ~20 min  ·  **still the important one**

Unchanged and still outstanding. This is the only thing standing between the
code and Phase 2's headline gate. The whole first-ascent pipeline is live and
tested; nothing can type a name without it.

**Follow `notes/phase2-naming-widget.md` §1–3.** In short:

1. Content Browser → **User Interface → Widget Blueprint** →
   `WBP_NameFirstAscent`.
2. Designer: Vertical Box centred, with `HeaderText`, `LineText`, `NameBox`
   (an **Editable Text (Box)**, tick **Is Variable**), and `ConfirmButton`.
3. Graph: `Event Construct` → cast to DirtbagGameInstance → `Get Naming Line
   Text` → `Set Text` on `LineText`.
4. `On Clicked (ConfirmButton)` → cast → **`Name First Ascent`**, with
   **Board Index** wired from **`Get Naming Board Index`** — *never a typed
   number*, that is how you name the wrong route — and **Name** from
   `NameBox` → `Get Text` → `To String`.
5. Level Blueprint: `Event Tick` → cast → `Get b Naming Pending` → Branch →
   **Do Once** → `Create Widget` + `Add to Viewport`, reset **Do Once** from
   the Branch's False pin.
6. **`Set Input Mode UI Only`** + `Set Show Mouse Cursor`. Skip this and you
   will see the box and be unable to type in it. Everyone misses it.

**Test it fast:** wall at **Board Index 25** (*the slab right of the
pull-off*, V3), `Venue = Crag`, press **C** four times to clean, then climb.

---

## 2. Place the Shaded Cave  ·  ~10 min  ·  **the new one**

There is a second crag now: **eighteen rope pitches from 5.9 to 5.14a plus
three bolted projects**, north-facing, forty minutes up the hill. It is the
only rock worth walking to in high summer, because Roadside is east-facing
and bakes.

1. Drop a `Dirtbag Day Spot` somewhere up-hill from the Lot. Set
   **Arrive At = `Cave`** (the enum has a third entry now).
2. Drop two or three `Dirtbag Climb Wall` actors near it with
   **Venue = `Cave`** and **Board Index** 0, 4, and 14
   (*Cave Dweller* 5.9, *Belay Slave* 5.11a, *Shade All Day* 5.12a).
3. Keep their triggers clear of the Roadside walls'.

**What to look for:** the pitches are **16–20 moves** where a boulder is 6–8,
so an attempt takes noticeably longer and the pump bar is doing real work for
the first time. Clipping costs pump — more from a bad stance.

> **Known and deliberate:** there is no belay animation, no clipping
> animation and no lowering. Rope staging is deferred to MVP rung 2 in
> `concepts/DIRTBAG.md`. It will look like bouldering thirty metres up. The
> *sim* is what is being tested here.

---

## 3. Check what the new keys and lines say  ·  ~10 min

At a **crag or cave** wall, unchanged:

- **C** — clean. Works at the cave now too (it did not before today).
- **B** — beta.
- **E** — climb.

New Blueprint nodes worth dropping on the HUD while you are in there. All on
the **DirtbagGameInstance** unless noted:

| node | says |
|---|---|
| `Age Line` | "31 — still going up" |
| `Load Line` | "everything aches; this is the warning" |
| `Injury Line` | "a pulley in the ring finger — 3 weeks, if you are sensible" |
| `Kit Line` | "one pad, a board in the van, and the gym until the month runs out" |
| `Standing Line` | "the crag is closed. The signs went up on the gate." |
| `Sponsor Line` | "free shoes, and they want nothing" |
| `Career Epitaph` | the whole career in one line |

**`Load Line` is the one to actually wire.** It is the warning before an
injury, and measured over twelve seasons a climber who stops training at it
sends **38 against 24** and spends **55 days hurt against 1,467**.

---

## 4. Play a proper fortnight  ·  ~45 min  ·  **this is the actual goal**

Everything above is setup. This is the part only you can do.

Play **fourteen days in a row** without reloading. Wake, read the forecast,
decide, sleep. Let it go wrong.

Deliberately do at least once:
- **Take a first ascent and name it.** (Phase 2's gate.)
- **Walk up to the cave** and get on a pitch.
- Wait at the **fire** for a window rather than climbing immediately.
- Let the **van** break and bodge it rather than paying.
- Climb a day on **shot skin** and notice whether jugs feel like the answer.

---

## 5. What to send back

In priority order. "Annoying", "didn't notice", "felt good" are all useful.

1. **Did the naming land?** That moment is Phase 2's gate. Earned, or
   administrative?
2. **Does a pitch feel different from a boulder?** It should feel long and
   pumpy where a boulder feels sharp and short. If it just feels like a
   boulder that takes longer, the sim is not carrying the difference and the
   staging will not save it.
3. **Does waiting for a window feel like a decision or dead time?**
4. **Is a breakdown drama or noise?**
5. **Anything you wanted to do and couldn't.**

### And one real decision, which is yours

**Phase 3's gate is "the money loop pressures the climbing loop", and five
separate mechanics have now failed to close it** — bills, fuel, the kit,
physio, and sponsorship. A title sponsorship that pays you out of the
workforce entirely (**21% of days worked → 0%, $39,040 over five years**) and
takes ~27 good days a year leaves the send count **exactly unchanged**.

The reason is structural: **skin is the binding constraint, not money and not
time.** A season has 208 days with a window and you can only climb ~130 of
them before your skin is gone, so losing 27 of the other 78 costs nothing.

Two honest ways forward, and I have deliberately not picked one:

1. **Stop skin being the sole cap**, so time and money have something to
   compete for.
2. **Restate the gate.** "Sends per year" may be the wrong metric. A
   sponsored climber who never works again climbs the same amount and lives a
   completely different life — the year's *texture* changes totally while its
   outcome does not. That might be exactly right.

I lean to (2). Play the fortnight first — whether money *feels* like it
presses on you is worth more than another measurement.

`notes/phase3-economy.md`, `phase3-kit.md`, `phase4-sponsor.md` have the
numbers.

---

## Known and deliberate — do not report these as bugs

- **No rope staging.** No belay, no clipping, no lowering. Deferred.
- **The cave has no belayer requirement in-game.** `WillBelay` and
  `BurnsTheyWillHold` are built and tested; nothing calls them yet, so you
  can climb pitches alone.
- **No town.** Six venues with opening hours exist and are now callable
  (`Town`, `Venue Is Open`, `Open Venue For`), but nothing is placed.
- **The odd-jobs board, factions, the kit and sponsorship are callable but
  unplaced.** Every one has Blueprint nodes now; none has a door to walk
  through. Standing, closures, pads, the hangboard, the gym membership and
  sponsors therefore cannot happen to you while you play.
- **Nothing can injure you yet.** Training load accrues and `Load Line` will
  show it, but the thing that actually redlines a climber is the hangboard,
  and you cannot buy one yet.
- **Retirement is callable, not reachable.** `Retire And Pass It On` works
  and is tested; no UI offers it.
- **The naming widget is the only UMG in the project.** Everything else is
  the canvas HUD on purpose, at blockout quality.

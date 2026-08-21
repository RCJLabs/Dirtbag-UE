# When you get home — the whole list, in order

**Rewritten 2026-08-21 (evening)** against what is actually in the build.
Roughly **90 minutes** if nothing fights you. Step 1 is the only one that
must go first; after that §2 and §3 are independent.

Everything below has been checked against the source today. Where a step
names a field, that field exists and is spelled that way.

---

## 0. Get the build green · ~10 min

1. **Close Unreal and Visual Studio completely.**
2. **GitHub Desktop → Fetch origin → Pull.** Top commit should be
   *"Make the pad a trade, and make the shop say what it costs"* or later.
3. **Delete** `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Binaries`.
4. Double-click `DirtbagUE.uproject` → **Yes** to rebuild. 2–5 minutes.

> **This pull is large but low-risk.** It is mostly wiring: fourteen sim
> functions that existed and were called by nothing are now called. No new
> sim files, no new bridge files. Everything checkable without Unreal is
> green — **62,169 harness checks**, unity build clean, and six checkers
> (definitions, bridges, field names, mirror coverage both directions,
> engine reachability, dials).

**Your save will migrate.** It is v13; the build is **v15**. Two migrations
run on load (`secrets`, `sponsor.hurtdays`) and both fill in zeros, which is
the literal truth for a career that predates those systems. Nothing to do.

**If it fails:** `Saved\Logs\DirtbagUE.log`, bottom of file, send me the
lines around the first `error`.

---

## 1. What will look different immediately · ~5 min, just read it

You do not have to do anything for these. They are what today changed, and
knowing what to expect is what makes step 3 useful.

| you will see | because |
|---|---|
| **Driving now costs money** — *"Drove to the crag. 30 minutes and $4 gone."* (at $9/hour) | `FuelFor` was never called by the game; driving was free |
| **A salaried day eats itself at dawn** — you wake up and it is 5pm | the trap existed only in the probe |
| **Sponsor news in the morning**, monthly money and a yearly verdict | never paid, never reviewed, in game *or* probe |
| **Dead rubber actually hurts**, and the shop says by how much | shoe wear was zeroed by the bridge on every attempt |
| **The gear shop sells crash pads** | the pad was reachable only from a Blueprint call |
| **Your first ascents now move the Lot's opinion of you** | `CreditFirstAscent` had no caller anywhere |
| **A load line, an injury line, standing, ethics news** on the HUD | seven lines existed and were drawn nowhere |

---

## 2. Place the Shaded Cave · ~30 min · **the one real build job**

Nothing about the rope, the runout, the clipping pump or the belayer is
reachable in game until this exists. It is the last thing blocking four
sim functions and an entire discipline.

### 2a. The drive there

1. In the level, **select the existing Travel spot** that drives to the crag.
   **Ctrl+W** to duplicate it. Move the copy somewhere sensible near the van.
2. On the copy, set:
   - **Kind** = `Travel`
   - **Travel Name** = `the cave`
   - **Travel Hours** — leave it alone. **Outdoors this field is now
     ignored.** The guidebook owns the approach (Roadside 0.5, the Cave 0.7,
     the Terrace 0.9) and the spot asks it, so a crag can no longer be forty
     minutes away in the book and half an hour away in the level. The field
     still applies to the gym and the town, where there is no rock.
   - **Arrive At** = `Cave`
3. **Travel Target** — this is `EditInstanceOnly`, so set it in the level, not
   on a Blueprint. Point it at wherever you want to stand at the cave.
4. **Duplicate it again for the way back**: Travel Name `the lot`, Arrive At
   `Crag`, Travel Target pointed at the van. Point the two at each other.

> Miss the return spot and you can drive to the cave and never leave. Ask me
> how the gym spot got its pair.

### 2b. The walls

Duplicate three crag walls and set **Venue = `Cave`** on each. That one field
is what makes them read the cave's guidebook instead of Roadside's.

Board indices, from the real generated book (seed `gym-1`):

| Board Index | line | grade | why this one |
|---|---|---|---|
| **3** | The Warm-Up Lap | **5.10c** | something you can actually do on day one |
| **8** | The Pump Clock | **5.12a** | at your limit — this is where the pump bar earns its keep |
| **18** | *the bolted line through the roof* | **5.13c** | **an open project** — clean it, work it, name it |

All 21 lines are rope routes. There are **no boulders in the cave**, which is
the point of it.

### 2c. Check it worked

Walk up to index 8. The approach toast should name the route and the grade.
Press **E**. If the pump bar climbs faster than it does on a boulder and you
get a rest stance partway, the rope sim is live.

---

## 2d. The Sun Terrace · ~10 min · new

Same two jobs as the cave, one venue along.

1. **Travel spot**: duplicate one, `Kind=Travel`, `Travel Name=the terrace`,
   **`Arrive At = Terrace`**, `Travel Target` set in the level. **And the
   pair back**, as before. Travel Hours is ignored outdoors — the guidebook
   says 0.9 and the spot asks it.
2. **Walls**: duplicate crag walls, **`Venue = Terrace`**. Board indices
   from the generated book:

| Board Index | line | grade | why |
|---|---|---|---|
| **2** | One O'Clock Sun | V5 | the classic, three stars |
| **7** | Day Off Work | V8 | at the top of what you can do |
| **11** | *the prow above the terrace* | V10 | an open project |

**It is a winter crag.** In midwinter it has more windows than anywhere else
(42 against the cave's 28) and they land at about **1:45pm** — so if you
test it in summer it will look like a worse Roadside, which is correct and
not a bug. Sleep to a winter day, or start a save on day 1, if you want to
see what it is for.

---

## 3. Play a fortnight · ~45 min · **this is the actual goal**

Fourteen days, no reloading. Wake, read the forecast, decide, sleep. Let it
go wrong.

**Do these at least once each:**

1. **Go to the gear shop with worn shoes.** Read the line. It will say what
   the rubber is costing you in grades.
2. **Go back when your shoes are fine.** It will offer you the second crash
   pad instead: *"A second pad, $260. Better landings, and one less reason to
   be brave."* — **this is the Phase 3 gate and the only part of it I cannot
   test.** See §4.
3. **Take a salaried job**, then try to climb the next day.
4. **Drive to the cave and get on a pitch.**
5. **Take a first ascent and name it** — then walk away and come back, and
   check the wall still calls it by your name.
6. **Climb until the load line goes red**, then decide whether to stop.
7. **Let the van break** and bodge it rather than paying.

---

## 4. The one question only you can answer

**Criterion 3 of Phase 3's restated gate is "the trade is legible while you
are choosing".** I can measure that the pad is worth about a send a year and
costs you a third of your head training. I cannot measure whether that reads
as *a decision* when you are standing in the shop with $300.

So, at the pad prompt:

- Did you **hesitate**?
- Did *"one less reason to be brave"* land as a real cost, or as flavour text?
- Would you have wanted to know **how much** head — a number — or would that
  have made it worse?

If you hesitated, criterion 3 passes and **Phase 3's gate is met** — and the
marker is yours to move, not mine. If it read as flavour, the line is wrong
and I will rewrite it.

---

## 5. What else to send back

In priority order. "Annoying", "didn't notice", "felt good" are all useful.

1. **Does a pitch feel different from a boulder?** Long and pumpy, against
   sharp and short. If it just feels like a boulder that takes longer, the
   sim is not carrying the difference and no amount of staging will save it.
2. **Does the fuel cost register**, or is it noise on a toast you skim?
3. **Is the salaried day a trap or just an annoyance?** It is supposed to
   feel like the day was taken from you before you woke up.
4. **Did the naming land the second time?** It is in the guidebook now, not
   just the ledger.
5. Anything you wanted to do and couldn't.

---

## Still open, and deliberately not done

- **`WillBelay`, `BurnsTheyWillHold`, `BestBelayer`, `BelayText`** — the
  belayer system. Held until the cave exists, because wiring a belayer picker
  with nowhere to belay is guessing at a UI. **Once §2 is done, tell me and
  I will wire it.** That is the last of the fourteen.
- **Shoes as a real decision.** They are currently cheap enough to be
  reflexive — resole cheap and slightly worse against new and dear would make
  them a second live choice. Not built; your call whether it is worth one.
- **A social policy for the probe.** `ShareBeta` and `PsycheFrom` are wired
  into the game and not modelled in the season probe, so its numbers ignore
  what partners are worth. Fixing that means deciding what a simulated player
  does with an evening, which is a design choice rather than a bug.

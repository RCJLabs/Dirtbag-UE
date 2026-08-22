# When you get home — the whole list, in order

**Rewritten 2026-08-22** against what is actually in the build. Since the
last rewrite the sim gained eight systems and every one of them is playable
with **zero Blueprint work** — the fire and the shop got their verbs in
C++, the way the pad offer did. Your session is: pull, build, place two
crags, then *play*.

Roughly **2 hours**. §1 must go first; §2 and §3 are independent; §4 is
the point of the whole thing.

## 1. Get the build green · ~10 min

1. `git pull` on `claude/dirtbag-unreal-port-ggvybe`.
2. Right-click `DirtbagUE.uproject` → Generate Visual Studio project files.
3. Build. Three days of container-green C++ land at once; the include
   checker has been over it, but signatures are still only provable on
   your compiler. **If it fails, paste me the full log before touching
   anything.**
4. Load your save. It migrates v13 → **v20** in one pass, **in memory**.
   The Output Log now says exactly what happened (`LogDirtbagSave`):
   *"file is v13, migrated to v20 in memory. The file itself updates at
   the next sleep."* — and that last clause is the part that looks like a
   bug and is not: **the file on disk keeps saying `version=13` until you
   sleep in game**, because the only thing that ever rewrites it is
   SaveNow, and a load must never touch the only copy. Sleep once, then
   the file reads v20 with all the new fields. Everything you had — the
   book, Bouncin, the bonds — should still be there after a sleep;
   **if anything is missing, stop and tell me; do not overwrite the
   save.**

## 2. What looks different the moment you press Play · just read this

- **HUD, bottom left**: up to three new slow lines — Dirtbag Years (after
  a year without the salaried job), what the town calls you (after a
  month of climbing with two people), and your dream once you own it.
- **Morning news lines**, said once each: a Dirtbag Year landing, the
  town naming your crew secondhand ("You heard somebody at the fire call
  you…"), a dream bought.
- **Your climber is no longer the template.** Fresh worlds and inheritors
  roll skills (±6) and body type from the seed. Reach bias is live —
  a Lanky climber and a Compact one own different cruxes.
- **You can get hurt outdoors now.** A cold crimp at your limit can pop a
  pulley on any burn — warm up before pulling on the project. Mileage
  days on jugs stay safe.

## 3. The gear shop grew a counter · ~5 min to try

Walk up to the shop:

- **Nothing chosen yet** → the prompt asks *"What is the money for?"* —
  press **1** (the Rig, $9,000), **2** (the War Chest, $14,000) or **3**
  (Home Base, $30,000). **Once, and it holds** — the other two stop being
  for sale for this whole career. Choosing is free; it just closes doors.
- **Chosen, not bought** → the prompt tracks it ("$3,240 of $9,000"), and
  the day you can cover it, **E buys it** and tells you what is left to
  your name. Worn shoes still outrank the dream at the counter.
- The HUD's saving-for line carries the same number everywhere else.

## 4. The fire deals now · ~10 min to try

At the fire, **E still sits** (rapport, psyche, talk). New: the prompt
names **what is out tonight** — cards, dice or blackjack, rotating with
the day, because you join what is being played.

The grammar is two keys: **C commits, F backs down.**

- **Cards (poker)**: C deals — you see your hand as a number and *them*
  as sentences ("Margo is not even looking at her cards"). C stays
  ($20 on top of the $5 ante), F throws them in (ante only). The reads
  sharpen with rapport: the better you know the Lot, the more the game
  pays. Measured: never folding loses ~$5 a hand; reading friends wins
  ~$10.
- **Dice (liar's dice)**: C deals — your five dice, somebody's bid, and
  what they looked like saying it. C calls the lie, F lets it go round.
  You cannot read a stranger; you can read a friend.
- **Blackjack**: C deals, C hits, F sticks and settles. Nobody in it but
  you and the deck — the game for a climber who knows nobody yet. Played
  well it costs the ante and nothing more.

One known blockout edge: if you ever place a **second** fire spot,
blackjack's live hand is shared between them. One fire per level for now.

## 5. Place the Shaded Cave · ~30 min · **the one real build job**

Nothing about the rope, the runout, the clipping pump or the belayer is
reachable until this exists.

1. **The drive**: duplicate the existing crag Travel spot. On the copy:
   `Kind=Travel`, `Travel Name=the cave`, **`Arrive At = Cave`**,
   `Travel Target` set in the level (it is EditInstanceOnly). Travel Hours
   is ignored outdoors — the guidebook owns the approach (0.7 for the
   cave). **Duplicate again for the way back** (`the lot`, `Arrive At =
   Crag`), and point the two at each other.
2. **The walls**: duplicate three crag walls, set **Venue = `Cave`**.
   Board indices from the generated book (seed `gym-1`): **3** (The
   Warm-Up Lap, 5.10c), **8** (The Pump Clock, 5.12a — the pump bar's
   showcase), **18** (the bolted line through the roof, 5.13c, open
   project). All 21 cave lines are rope; no boulders, on purpose.
3. **Check**: at index 8 the approach toast names route and grade; press
   E; the pump bar should climb faster than on a boulder, with a rest
   stance partway.
4. **The belayer refusal**: try to start a pitch when nobody at the Lot
   will hold your rope (early, before rapport). The refusal line should
   name the problem. This is the last of the fourteen unwired functions
   that has never been seen on screen.

## 6. The Sun Terrace · ~10 min

Same two jobs, one venue along: Travel pair with **`Arrive At =
Terrace`** (guidebook says 0.9 hours), walls with **`Venue = Terrace`**,
board indices **2** (One O'Clock Sun, V5), **7** (Day Off Work, V8),
**11** (the prow above the terrace, V10, open project).

**It is a winter crag.** Test it on a summer save and it will look like a
worse Roadside — that is correct. Its 42 winter windows land around
1:45pm, which is what a nine-to-five steals.

## 7. Then play — and these are the questions · ~45 min

Play a real stretch, a fortnight or more, and come back with answers to
whichever of these the session touched:

1. **Brush a project you cannot climb yet.** The toast should say *"Word
   gets round. Nobody at the Lot will touch it now."* exactly once. This
   is the strategy that triples your first ascents, and it is the biggest
   thing playtesting can validate right now: **do you believe the claim
   when the game says it?**
2. **Choose a dream and live with it a while.** Does the choice feel
   like a commitment or a menu? (If it needs to be revocable, that is a
   design conversation, not a bug report.)
3. **Play the fire on three different nights.** Does one game earn its
   place worst? §4 of the old plan said keep one; you said keep three;
   a fortnight of evenings will say who was right.
4. **Get hurt, or notice you did not.** The tweak is tuned to a handful
   per thirty years — across one fortnight you will probably see nothing,
   which is correct. What you might see: the warm-up mattering.
5. **The naming widget, the climb wall, the mannequin yaw** — all fixed
   or built last round; anything that regressed, say so.

## 8. What is deliberately not done

- **The Lot never varies** — the same three neighbours across ninety
  years. Known, logged, and a design question for you, not a dial.
- **Poker/dice/blackjack table UI** is toasts. If an evening of it feels
  good, it earns a widget; if not, we saved one.
- **Early Access decision** — Phase 4 says it happens at its start. It
  has not happened. It is also unblockable by anything in a container.

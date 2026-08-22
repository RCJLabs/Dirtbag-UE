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

## 4. The fire has a table now · ~10 min to try

At the fire, **E still sits** (rapport, psyche, talk). It refuses while
you are holding a hand — *"Finish the hand first"* — because sitting
passes hours and hours cross midnight, which would delete the hand.

Walk into the fire's trigger and **a panel comes up** where the climbing
session bar normally sits: tonight's game, your hand, the reads, the pot,
the stake, and the two verbs. It stays there while you think. **This
replaced the toasts**, which expired in twelve seconds and took the reads
with them.

The grammar is still two keys — **C commits, F backs down** — and now
they are *named on screen* for whichever game is out.

- **Cards (poker)**: C deals. Your hand is a **bar**; the Lot are
  sentences ("Margo is not even looking at her cards"), listed under it
  and kept there. C stays in, F throws them in (ante only).
- **Dice (liar's dice)**: C deals — your five dice, somebody's bid, and
  what they looked like saying it. C calls the lie, F lets it go round.
- **Blackjack**: C deals, C hits, F sticks and settles. The bar is your
  total **against 21**, so it says how close to the edge you are. Nobody
  in it but you and the deck.

**New: 1/2/3 set the stake** — the ante, half the ceiling, the ceiling
($5 / $20 / $40). Same three keys as the shop's dream counter, same
question in both places. Poker and dice let you size the bet *after* you
have seen the hand, because that is the decision; blackjack locks at the
deal.

The heading counts the evening: **how many hands and what you are up or
down**, which nothing tracked before and which is the number that should
decide whether there is a next hand.

**Two things about the balance changed, and both are one dial if you
disagree** (`notes/phase5-the-fire-table.md` has the tables):

- Poker **pays about a quarter of what it did**. Betting the ceiling used
  to be free money — it won at every rapport by a factor of ten, so
  "press 3" was the whole strategy — and the Lot now folds harder against
  a big raise. A friend playing the read goes from ~$10 a hand to ~$2.60.
- **A stranger now loses money at poker** (~-$0.80) where it used to pay
  ~+$3. Liar's dice already charged a stranger $7.69 for the same
  ignorance; poker was contradicting its own table.

Walking out of the trigger mid-hand now **folds** rather than being a free
look, and toasts the result, since the table goes with you.

**One thing to re-check in the details panel:** the fire spot's
`CardStake` is gone, replaced by **`StartingStakeNotch`** (0/1/2, default
1 — the old $20). Nothing needs re-placing; the default is the old
behaviour.

One known blockout edge, unchanged: if you ever place a **second** fire
spot, blackjack's live hand is shared between them. One fire per level for
now.

## 4b. The prompts stay on screen now · just read this

Walk into any trigger — a wall, the shop, the van, the fire — and **a
panel appears along the bottom** saying what the keys do there. It stays
for as long as you are standing in it, and rebuilds itself when anything
it describes changes (buy the shoes, and the counter immediately starts
pitching the pad instead).

This replaced the four-second prompt toast. Nothing to place or wire.

**The one thing worth testing on purpose:** walk up to a **roped** line at
the gym while tired. You should see **both** the belayer line and the
body-state advice at once. Before this, the two went through the same
keyed toast slot one statement apart, so the second silently replaced the
first and you were **never told who was holding the rope** — in exactly
the case where it mattered.

Also on the climbing panel now: **"attempt 14"** beside the route name for
the whole go rather than a four-second flash at the start, and the HOLD
Space reminder sits under the grip bar until your first successful latch,
then never again this session.

Two red messages you should never see unless a level is placed wrong —
*"SETUP: <actor> has no HoldLine spline points"* and *"SETUP: <actor> has
no Travel Target"* — now last thirty seconds and also go to the Output Log
under **LogDirtbagSetup**, so a playtest cannot hide them from you.

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
5. **The mannequin faces the rock** — verified at the desk 2026-08-22,
   with the derived facing and `MeshForwardYaw = 90` (so the UE5
   mannequins are indeed authored facing +Y; that default is now a
   measured fact rather than a guess). **The naming widget and the climb
   wall** — fixed
   or built last round; anything that regressed, say so.

## 8. What is deliberately not done

- **The Lot never varies** — the same three neighbours across ninety
  years. Known, logged, and a design question for you, not a dial.
- **Poker/dice/blackjack table UI** is toasts. If an evening of it feels
  good, it earns a widget; if not, we saved one.
- **Early Access decision** — Phase 4 says it happens at its start. It
  has not happened. It is also unblockable by anything in a container.

# When you get home — the whole list, in order

**Rewritten 2026-08-23 (second time today).** The previous version said
**save version v21**. It is **v31**. Ten save versions, six commits, and
four phases of work landed since it was written, and it does not mention
comps, the circuit, the national team, the World Cup, the Games, leagues,
or anything at all about being ill or having teeth.

That is a rewrite, not an edit — same reason as last time, and the same
lesson: **this file goes stale faster than anything else in the repo,
because it is the only file whose job is to be current.**

Two things changed shape:

- **§7's Lot blockout is unchanged and is still the point.** Nothing in
  four phases of sim work touched it, because none of it could.
- **Everything else on this list is now a much longer read**, because the
  indoor half of a climbing career got built: gym comp → circuit → national
  ranking → the national team → the World Cup → the Games, plus leagues,
  plus the whole medical file.

Roughly **3 hours**, of which §8 is two of them and is the point. §1 must go
first.

---

## 1. Get the build green · ~10 min

1. `git pull` on `claude/dirtbag-unreal-port-ggvybe`.
2. Right-click `DirtbagUE.uproject` → Generate Visual Studio project files.
3. Build. **Six commits of container-green C++ land at once**, including
   three new sim modules (`DirtbagWorldStage`, `DirtbagLeague`,
   `DirtbagMedical`, `DirtbagAilments` — four, actually). All twelve
   preflight checkers pass, including a unity build of the whole sim as one
   translation unit at **35 files**. Signatures are still only provable on
   your compiler. **If it fails, paste me the full log before touching
   anything.**

4. **Load your save. This is the one to read carefully.**

   **Save version is now v31**, up from v21. Ten migrations run in
   sequence, and **nine of them are uneventful**. One is not:

   > **v27 → v28 deliberately throws your ranking away.**

   You will not have noticed a ranking, because comps did not have a door
   until this week — so in practice this costs you nothing. But the reason
   matters and I want it on the record rather than discovered: the old
   ranking was a **lifetime total** on a curve where coming mid-field paid
   half a win. A measured ten-year career reached **14,633 points against a
   top tier of 2,200.** The new ranking is a **rolling twelve-month
   record**. The two are not the same measurement at different scales, they
   are different measurements — carrying the old number across would have
   handed you a World-Class rung you could never lose, because there is no
   record behind it to age out.

   A migrated career loads **Unranked** and re-earns its rung over its next
   season of comps, which is about eleven weeks of play.

   Everything else migrates to exact rather than generous: you were never on
   the national team, never at a World Cup, never at the Games, have never
   been to a league night, have a clean medical file with no cortisone in
   any joint, and have good teeth.

   **One case worth knowing**: if your climber was mid-injury when you last
   saved, the injury survives and the new staged comeback picks it up the
   next morning. That is a case I built a guard for specifically, and a test
   pins it.

   Same rule as always: it migrates in memory, the file keeps saying v21
   until you sleep, and **if anything is missing, stop and tell me; do not
   overwrite the save.**

---

## 2. Eight new keys · read this before you play

All C++ on existing trigger volumes. **No Blueprint work, nothing to place,
nothing to wire.** They only respond where they mean something.

**At the gear shop counter** — this is now the care counter as well:

| key | what |
|---|---|
| **V** | have it looked at — first press a physio ($60), second the scan ($340) |
| **C** | a cortisone shot ($180) |
| **O** | the operation (needs the scan, needs it to be bad enough) |
| **N** | push on to the next stage of the comeback |
| **B** | buy or cancel health insurance |
| **K** | take something for it, when you are ill ($11) |
| **Y** | deal with the tooth |
| **Z** | see somebody about your head ($110) |

**At the van:**

| key | what |
|---|---|
| **X** | twenty minutes of prehab |

**At the gym wall**, `E` now means four different things depending on what
is on: the Games, a World Cup round, the Tuesday comp, or the Wednesday
league night — in that order, because that is the order the day matters. You
will not have to think about it; only one of them is ever on.

---

## 3. The indoor career now exists · ~30 min to see the bottom of it

This is the biggest single addition since you last sat down, and the fastest
way to meet it is to **go to the gym on a Wednesday**.

**Leagues (E at the wall, on league night).** Five dollars, five problems,
**ten goes** — more than a comp gives you, because a league night is a
session with a scorecard rather than a test. It is worth **no ranking points
at all** and that is the design, not an omission. What you chase is your own
best score: it only goes up, nobody can take it off you, and beating it by
one point on a Wednesday in a bad year is still beating it. Eight weeks make
a block with a table and a small prize.

Measured over thirty years: about **one personal best a year**, and a
quarter of blocks won by a climber who wins one domestic comp in 253. The
league is the one room you can actually win.

**Comps (E at the wall, when the poster says so).** Five problems, seven
goes, $20. **At Regional and above it is a whole day**: qualification, a
semi-final, a final. Six come out of quals and four out of the semi, the
final is four problems and five goes, and **the score does not carry** — a
good qualification buys you a place in the semi and nothing else. Being
knocked out is a result, not an error, and the game says which round you
went out in.

**The ranking, and what it gates.** Unranked → Regional Climber → National
Prospect → **National Team (700)** → **Olympic Hopeful (1200)** →
World-Class. It is a rolling twelve-month record, so **stop competing and
you come off the team**, exactly as you would.

**The national team.** Named at 700, held down to 560, cut below that. Five
teammates who are the people you have been chasing up the standings, a head
coach with opinions, **$900 a year that does not cover rent**, and a
committee that sits **once a year** and does not explain itself.

**The World Cup.** Ten real venues with real travel costs — Salt Lake $210,
Wujiang $800 — and the federation pays the entry but not the flight. Six
rounds a season. **The field flies whether you do or not**, so a round you
skip is not a round that did not happen, it is twelve other people banking
while you were at home. The season is decided by which rounds you could
afford.

**The Games.** A four-year cycle. You need 1,200 ranking points to be there
at all, and the seven you are up against are the best in the world. A
thirty-year career sees seven of them; most see two or three.

**The one design call worth knowing about**, because it is the difference
between the gym and the world: a gym comp is set at *your* grade plus a tier
offset — it follows you up as you improve. The World Cup is set where it is
set. Getting better is what closes the gap. (Reading it the other way made
the whole top of the ladder unwinnable at every skill level in the game, for
everybody, and every one of 76,000 checks passed. It took a measurement to
find.)

---

## 4. Being hurt is no longer a wait · ~20 min, and this is the best thing to report on

The old injury was eleven days, physio buys six back, nothing to decide.
Now:

> **An injury hides its grade until you pay to look at it.**

You get a sore finger and a sentence that says *"Something in the finger.
You do not know how bad."* Sixty dollars buys a physio's hands — **close and
not always right**, and the error is the whole mechanic. Three hundred and
forty buys the machine, which is exact and is the only thing that unlocks an
operation, because nobody operates on a guess.

Then **the comeback is staged** — resting, moving it, a graded return — and
it advances **because you press N**, not because a timer ran out. Waiting a
stage out is never a gamble. Pushing on early is, and the odds get worse the
earlier you go: three days early you usually get away with it, the morning
after a bad one you mostly do not.

**The shot (C) is the tempting wrong answer.** It ends the acute stage
outright — you are climbing this week — and marks the joint permanently.
Four in one joint is a different joint. Surgery is the only thing that ever
takes damage back off, and it costs $2,200 and most of a season.

**Insurance (B) is a real bet, and here is the number.** Thirty years of
premiums is about $6,250. Measured claims ran from $816 on a lucky body to
$8,704 on an unlucky one. So it is right on the career that got hurt a lot
and a slow expensive mistake on the one that did not.

**But the sharpest thing this system does is about money, not medicine.** A
career earns about $7,200 a year and ends thirty years of it with $159 in
the tin. A probe policy that *tried* to scan every injury spent **$0 in
thirty years uninsured** — it could never afford $340 at the moment it was
needed — and went untreated on all forty-one of them. Insurance is what puts
real medicine within reach of a dirtbag. That fell out of the measurement
rather than being designed in, and **it is the thing I most want you to
check against your own instinct for the game.**

---

## 5. And three things that are wrong with you that are not the injury · ~10 min

Every injury in this game is something you did. These are deliberately not.

**You get ill (K).** Two or three colds a year, and it is not a die: it
arrives off how you are living, so a fed, rested climber in a warm van
essentially never gets ill and a hungry one sleeping cold on a wrecked body
does. **Eleven dollars nearly halves the days you lose to it** — 633 down to
356 over thirty years — which is the cheapest good decision in the game and
is deliberately still a decision.

**Your teeth (Y), and this is the one to watch.** A twinge becomes an ache
becomes an abscess, on a clock, and **nothing improves it with rest.** The
only thing that ever has is money, and it costs more at every stage: $70 for
a filling, $780 for the root canal it becomes. It is the game's one pure
test of whether you will spend on something that is not climbing.

Left alone entirely, an abscess does eventually stop hurting — **the way it
stops is the tooth.** A career that never pays loses about six of them and
spends half its days with something wrong in its mouth. That number used to
be *all* of them: the first version never resolved, so a career sat at an
abscess for 10,688 days out of 10,950 and came away with four sends instead
of twenty-four. Bounded now.

**Prehab (X at the van).** Twenty minutes of a morning. It is a **streak,
not a total** — twenty minutes once is nothing — and at full strength it is
a third off the odds of a tweak and **never all of them.** It survives a few
missed mornings, because a habit you lose by going to a wedding is not a
habit.

**And the shrink (Z).** $110, once a fortnight, and it is the only thing in
this game that buys psyche back. It also does something a rest day cannot:
it moves where psyche drifts back to overnight, for a month.

---

## 6. Still outstanding · the camera · ~15 min, needs your eye

**Unchanged, and it has been the item most in need of a human for three
sessions running.** Every number is a first guess by something that has
never seen the shot. All `EditAnywhere` under **Dirtbag|Shot**:

1. Walk up to a wall — the shot should be **exactly as you left it**.
2. Press E on a tall line. Climber in frame to the top, rock above their
   hands. Too high or low is **CameraLead** (55).
3. Watch a crux. It should come in and *settle*, not snap. Too much
   movement: **CameraTightenBy** (0.32). Too sudden: **TensionEase** (1.8).
4. Watch something long and pumpy. Locked early, not quite still late.
   Nothing visible: raise **PumpSway** (7). Seasick: lower it.
5. If any of it is worse than the tripod: **bCameraFollows = false**.

**I cannot pass this gate.** It is Phase 0's, re-asked: *a watcher can tell
how close an attempt was without reading a number.* Get somebody who is not
you to watch an attempt with the HUD ignored.

---

## 7. Still outstanding · sound · optional

No audio in the project and **nothing needs assigning.** Slots on the wall
actor under **Dirtbag|Sound**, plus one `InteractSound` per day spot.

**Start with `BreathLoop`** — a calm two-or-three-second loop, driven by pump
for volume and pitch. It does more than everything else combined, because
pump is the number the session turns on and this is that number without a
bar. Then `MoveSound` and `LandSound`. `ChalkSound` last, and check it fires
*rarely* — if you hear it every move, drop `ChalkBelowOdds` (0.7).

Fire crackle, wind, the Lot at night are **`AmbientSound` actors in the
level**, not these slots.

---

## 8. The Lot blockout · ~2 hours · **still the whole point**

Unchanged from the last list, because nothing I have built since could touch
it. Repeated in full because it is the item that matters.

You said: *"i dont even know what assets to use or how to put things
together."* The answer to the first half:

> **For the blockout, none. Do not buy anything yet.**

The shopping list is written down (`notes/phase6-the-place.md` Part 1) and
**none of it is needed to build the Lot.** Buying before the blockout is how
you end up dressing a shape that turns out to be wrong.

Why the Lot first: **you sleep there every night of a thirty-year career.**

### 8a. Make ground · ~20 min
1. New Landscape actor. Sculpt roughly — **you are making occlusion, not
   terrain.** A hill between the Lot and the road is the entire job, because
   what you cannot see is what makes two places two places.
2. Flat pad for the Lot, big enough that crossing it takes a few seconds.
3. **Do not texture it.** Grey is correct.

### 8b. Move what you have onto it · ~30 min
The trigger volumes get **relocated**, at real distance from each other:
the van (sleep, and the hangboard on its side door, and now prehab), the
fire, the dog bowl, the travel spot at the edge, a rest spot by the pads.

**The distances are the design.** If the fire is two steps from the van,
sitting down at it costs nothing and the evening stops being a choice.

### 8c. Play it grey · ~20 min
Walk it. Sleep. Drive to Roadside. Climb. Come back.

**It will be grey and it will be a game.** That is the checkpoint. If it
feels wrong grey, no amount of Megascans fixes it and the layout needs
another pass.

### 8d. Only then, dress one pocket
One at a time, playing after each. The climb walls are last: a wall is a
spline plus a mesh and **the rock behind it is scenery**, so the rock
purchase blocks nothing. You could build the whole valley and play a season
before buying a cliff.

**The one thing expected to be genuinely hard** is making a hand-drawn hold
spline line up with real rock features. **Do one wall and find out how much
fuss it is before committing to twenty-five.**

---

## 9. Then play — and these are the questions

Ranked by how much I need the answer:

1. **Does the grey Lot feel like a place?** §8c. If yes, the rest of Phase 6
   is work rather than risk. Everything else on this list is a dial.
2. **Can you ever afford a scan?** §4. The whole medical system leans on
   cash-on-hand being the binding constraint. If it turns out you are richer
   than the probe thinks, half those prices are wrong.
3. **Does the tooth make you spend?** §5. It is meant to be the one thing
   you resent paying for and pay for anyway. If you ignore it and shrug,
   it is too weak; if you dread it, it is too strong.
4. **Play one comp at Regional.** §3. Three rounds, and the score does not
   carry. Does the semi feel like a second chance or like a chore?
5. **Skip a World Cup round you cannot afford.** §3. Does the table moving
   away from you land, or is it just a number going down?
6. **One league night.** §3. Is a personal best something you would come
   back for on a Wednesday?
7. **Camera**, §6, with somebody who is not you.

---

## 10. What is deliberately not done

- **Phase 6 is untouched by all of this.** Nine of eleven walkable zones are
  empty, the Lot is trigger volumes, and no amount of sim work changes it.
- **No audio assets.** §7.
- **Phase 7's `social` axis** is still unwired — it needs turnout, which
  needs somewhere for people to turn up to.
- **Phase 8's balance call is still open**: 502 races in thirty years is a
  lot. My reading is "fewer races, longer clock", but it is taste and it is
  yours.
- **Phase 11 (a life outside it), 12 (work as a craft) and 13 (trad)** are
  not started. All three are container work whenever you want them.

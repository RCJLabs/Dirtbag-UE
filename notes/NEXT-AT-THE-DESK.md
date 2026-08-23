# When you get home — the whole list, in order

**Rewritten 2026-08-23.** The last version was written on the 22nd and is
**eleven commits stale** — it does not mention ethics, the jobs board,
sponsorship, physio, membership, the salaried job or the wider map, because
none of them existed when it was written. Grep the old one for `physio` and
you get nothing. That is why this is a rewrite rather than an edit.

Two things changed shape since then, and they change what this session is
for:

- **Everything that was a build job last time is done.** The Shaded Cave and
  the Sun Terrace are placed; you played it and said it works. §5 and §6 of
  the old list are retired.
- **The place is now the only thing left that needs you.** Phases 7–13 in
  the roadmap are all sim work I can do without you. So this session is
  **the Lot blockout**, and everything above it is checking that three days
  of C++ landed intact.

Roughly **2½ hours**, of which §7 is two of them and is the point. §1 must
go first; §2–§6 are just reading and pressing keys.

---

## 1. Get the build green · ~10 min

1. `git pull` on `claude/dirtbag-unreal-port-ggvybe`.
2. Right-click `DirtbagUE.uproject` → Generate Visual Studio project files.
3. Build. **Eleven commits of container-green C++ land at once.** All eleven
   preflight checkers pass, including a unity build of the whole sim as one
   translation unit — but signatures are still only provable on your
   compiler. **If it fails, paste me the full log before touching
   anything.**
4. Load your save. Save version is still **v20** — nothing since the 22nd
   has changed the save shape, so if you already slept once after the last
   session, this load is uneventful. If you have not, the same rule applies
   as before: it migrates in memory, the file on disk keeps saying the old
   version until you sleep, and **if anything is missing, stop and tell me;
   do not overwrite the save.**

---

## 2. Nine keys, and five of them are new · read this before you play

This is the biggest change since you last sat down. Every one of these is
C++ on a trigger volume — **no Blueprint work, nothing to place, nothing to
wire.** They only respond where they mean something.

**At a day spot** (the counter, the fire, the van, the shift desk):

| key | where | what |
|---|---|---|
| **E** | everywhere | the obvious verb for that spot |
| **C** / **F** | the fire | commit / back down |
| **1 2 3** | shop, fire | pick a dream · set the stake |
| **G** | anywhere | the guidebook |
| **R** | the van | retire, and hand the valley on |
| **S** | the shop counter | **sign with a sponsor** ← new |
| **P** | the shop counter | **see a physio** ← new |
| **M** | the shop counter | **buy or renew gym membership** ← new |
| **H** | counter · van | **buy a hangboard · use it** ← new |
| **J** | the shift desk | **take (or quit) the salaried job** ← new |

**At a climb wall:**

| key | what |
|---|---|
| **E** | start an attempt |
| **Space** | HOLD TO CLIMB |
| **C** | clip (rope) |
| **B** | brush |
| **G** | the guidebook |
| **T** | **do something you would not admit to** ← new |
| **1 2 3 4** | which shortcut ← new |

Four things worth knowing about that list rather than discovering:

**H is one key in two places on purpose.** At the counter it buys the
hangboard; at the van it uses it. Where you are standing says which you
meant.

**S, P and M are not E.** They are their own keys because E buys rubber, and
signing away three days a month, buying six days off an injury and renewing
a membership should not be the same press as buying shoes.

**T is offered without being urged.** You get a bare `(T)` on the end of the
route line and never a sentence explaining what it would buy. Press it and
it says *"Nobody is watching."* and what the line will take. **T again backs
out; walking away withdraws it.** Nothing congratulates you.

**J has no ceremony and no warning.** Taking the salaried job is entirely
reasonable, which is the whole design of the trap, so the game reports what
happened — including the Dirtbag Year streak it just ended — and says
nothing about whether it was wise.

---

## 3. Four systems that existed and could not be reached · ~20 min to try

Each of these was built, tested, measured and saved, and **had no door**.
They are worth trying deliberately because the doors are new and the systems
behind them are not.

**Sponsorship (S at the counter).** The shop mentions it only when the offer
beats what you hold. Every offer states **both halves** — *"a title
sponsor - $640 a month, and 3 days a month that will not be the rainy
ones"* — because an offer that says what it pays and not what it wants is an
advert. The obligation is then **taken at Sleep rather than offered**: you
wake up and six hours already belong to somebody. Measured over five years:
a title deal roughly doubles your income and takes about 15% of your good
days, and **every day it takes is a good day.**

**Physio (P at the counter).** The prompt states the trade in the only units
that matter — *"$95, and about 6 days off it"*. A refusal says which of the
three reasons it is. **Watch whether you can ever afford a full course**: a
thirty-day injury costs $285 all in, and the measured thirty-year saver
career peaks at **$447 cash ever held.** That may be exactly right —
*"I cannot afford the physio"* is the most authentic sentence in this game —
but nobody could know while the door did not exist. **This is the single
most useful thing you can report back.**

**Membership (M at the counter), and the gym was free.** `RenewGymMembership`
had no caller, and neither did the function that *checks* it — so the gym
let you climb indoors in any weather forever for **$0**, while
`Sim/DirtbagKit.h` calls the membership *"the load-bearing one, the first
recurring cost in the game."* **The wall is the gate now**: no membership,
no plastic, and it says so on approach rather than after you have crossed
town. Rock is unaffected.

**The jobs board (E at the shift desk, then 1/2/3).** The desk used to say
*"Take a shift?"* and give you a flat $60. There are three gigs a day now
and they are not interchangeable: flyering is **$11.67/hour** and shooting
guidebook photos is **$65/hour** — five and a half times better, two hours,
almost no energy, and **the one that turns the valley against you.** E shows
the board; the numbers take a gig; E deliberately does not, because a key
that silently picked the best would hand you a standing hit you never chose.

---

## 4. The map went from five zones to sixteen · nothing to do, but read it

`DestinationZone` on your travel spots now has **sixteen** entries in the
dropdown instead of five.

**Nothing you have placed has moved.** The Lot, Town, Roadside, Cave and
Terrace keep ordinals 0–4 and always will — new zones only ever go on the
end, because that property stores its value as a number and inserting one
anywhere else would silently repoint every travel spot in your level. There
is a test pinning those five ordinals for exactly that reason.

What is new in the list: Old Town, Midtown, Trailhead, Outskirts, Uptown,
Market Row, Grand Plaza, Greenwood Park, Trout Lake — **eleven walkable
zones in one connected grid** — plus the **Olympic Village** and **the
farm**, which are van-only like the crags.

Two consequences:

- **`Town` now displays as "Downtown".** Same zone, same ordinal; the word
  changed because there are seven districts now and "town" stopped being a
  place.
- **Walks are priced off the real grid.** The Lot and downtown are diagonal,
  so that walk is two borders — and the border cost was set to ten minutes
  precisely so the walk you had tuned to twenty still costs twenty. Nothing
  you have should feel different.

None of the nine new zones has any content. That is fine and expected: they
get a walk-through and a name until something wants to live there.

---

## 5. Still outstanding from last time · the camera · ~15 min, needs your eye

Unchanged and still the item most in need of a human. Every number is **a
first guess by something that has never seen the shot.** All `EditAnywhere`
under **Dirtbag|Shot**:

1. Walk up to a wall — the shot should be **exactly as you left it**.
2. Press E on a tall line. The climber should stay in frame to the top, with
   rock above their hands. Sitting too high or low is **CameraLead** (55).
3. Watch a crux. The shot should come in and *settle*, not snap. Too much
   movement: **CameraTightenBy** (0.32). Too sudden: **TensionEase** (1.8).
4. Watch something long and pumpy. Early it should be locked; late it should
   not quite hold still. Nothing visible: raise **PumpSway** (7). Seasick:
   lower it.
5. If any of it is worse than the tripod: **bCameraFollows = false**.

**The gate here is not something I can pass.** It is Phase 0's, re-asked:
*a watcher can tell how close an attempt was without reading a number.* Get
somebody who is not you to watch an attempt with the HUD ignored.

---

## 6. Still outstanding from last time · sound · optional

There is still no audio in this project and **nothing needs assigning.** The
slots are on the wall actor under **Dirtbag|Sound**, plus one
`InteractSound` per day spot.

**Start with `BreathLoop`** — a calm two-or-three-second loop, driven by
pump for volume and pitch. It does more than everything else here combined,
because pump is the number the session turns on and this is that number
without a bar. Then `MoveSound` and `LandSound`. `ChalkSound` last, and
check it fires *rarely* — if you hear it every move, drop `ChalkBelowOdds`
(0.7).

Fire crackle, wind, the Lot at night are **`AmbientSound` actors in the
level**, not these slots.

---

## 7. The Lot blockout · ~2 hours · **the whole point of this session**

You said: *"i dont even know what assets to use or how to put things
together."* Here is the answer to the first half, and it is short:

> **For the blockout, none. Do not buy anything yet.**

The shopping list is decided and written down (`notes/phase6-the-place.md`
Part 1) — a stylized modular character pack, two or three Megascans cliff
sets, a campervan with a good interior, the DOG pack, and Epic's free Game
Animation Sample. **None of it is needed to build the Lot**, and buying
before the blockout is how you end up dressing a shape that turns out to be
wrong.

Why the Lot first: **you sleep there every night of a thirty-year career.**
It gets more screen time than anything else in the game by an order of
magnitude. The Cave and the Terrace are seasonal and can stay grey for
months.

### 7a. Make ground · ~20 min

1. New Landscape actor. Sculpt roughly — **you are making occlusion, not
   terrain.** A hill between the Lot and the road is the entire job, because
   what you cannot see is what makes two places two places.
2. Flat pad for the Lot itself, big enough that walking across it takes a
   few seconds and not one.
3. **Do not texture it.** Grey is correct at this stage.

### 7b. Move what you already have onto it · ~30 min

Your trigger volumes are not thrown away — they get **relocated**. This is
the step that turns a game in a test level into a game in a place.

Put them where they would actually be, at real distance from each other:

- **the van** — sleep spot, and the hangboard hangs over its side door
- **the fire** — far enough from the van that walking over is a decision
- **the dog bowl** — by the van
- **the travel spot out** — at the edge, where a road would leave
- **a rest spot** — the log by the pads

The distances are the design. If the fire is two steps from the van, sitting
down at it costs nothing and the evening stops being a choice.

### 7c. Play it grey · ~20 min

Walk the Lot. Sleep. Drive to Roadside. Climb. Come back.

**It will be grey and it will be a game.** That is the checkpoint — if it
feels like a place while it is untextured boxes, dressing it will work. If
it feels wrong grey, no amount of Megascans fixes it, and it is the layout
that needs another pass.

### 7d. Only then, dress one pocket · the rest of the session

One pocket at a time, and **play it again after each.**

The climb walls are safe to leave until last, and this is worth
understanding because it removes the scariest purchase from the critical
path: `ADirtbagClimbWall` is a spline of hold positions plus a mesh that
interpolates along it, and **the rock behind it is scenery.** A wall works
on a grey box and keeps working unchanged when a cliff arrives behind it —
you place the cliff, then drag the spline points onto features that look
like holds. **The rock purchase is not blocking anything.** You could build
the entire valley and play a full season before buying a single cliff.

**The one thing expected to be genuinely hard** is exactly that last step:
making a hand-drawn hold spline line up with real rock features. **Do one
wall and find out how much fuss it is before committing to twenty-five.**

---

## 8. Then play — and these are the questions

Whichever the session touches:

1. **Can you ever afford a full course of physio?** §3. The most useful
   single answer you can bring back.
2. **Does the board make you choose?** Do you ever take the $11.67 gig over
   the $65 one because of what the good one costs you with the valley?
3. **Sign a sponsor and live a month.** Do the taken days feel like a price
   or like a tax? They are always good days, by design.
4. **Press T once.** Not because you should — because I want to know whether
   the offer reads as tempting or as a menu item.
5. **Brush a project you cannot climb yet.** The toast should say *"Word
   gets round. Nobody at the Lot will touch it now."* exactly once. Still
   the biggest claim in the game that playtesting can validate: **do you
   believe it when the game says it?**
6. **Does the grey Lot feel like a place?** §7c. If yes, the rest of Phase 6
   is work rather than risk.

---

## 9. What is deliberately not done

- **The Lot never varies** — the same three neighbours across ninety years.
  Known, logged, a design question for you rather than a dial.
- **Nine of the eleven walkable zones are empty.** By design for now.
- **No audio assets.** §6.
- **Comps, the ladder and the Olympics are back in scope** as of today
  (`concepts/DECISION-comps-are-back.md`) and are **Phase 9** — nothing to
  do at the desk, but the Olympic Village is now a zone, so leave room for
  it when you sculpt.

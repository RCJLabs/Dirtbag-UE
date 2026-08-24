# When you get home — the whole list, in order

**Rewritten 2026-08-24 (third rewrite).** The previous version said **save
version v31**. It is **v34**. Four more commits landed since it was written
and it does not mention trad, the buttress, the rack, habits, quirks, or the
narrator.

Same lesson as the last two rewrites: **this file goes stale faster than
anything else in the repo, because it is the only file whose job is to be
current.**

**Read this bit before anything else.** Since your last build, **1,063 lines
of engine C++ have been written and never compiled by anything.** The sim
half — 4,779 lines — is compiled here every commit by two compilers at two
language standards, and the thirteen preflight checkers pass. **None of that
is a UHT run and none of it is MSVC.** §1 is therefore the whole session's
risk, and it goes first.

What was checked here, so you know what is already ruled out:

| checked | result |
|---|---|
| Sim under g++ and clang, C++17 **and C++20** (UE 5.8's standard) | clean, `-Wall -Wextra` |
| Sim as one translation unit, 40 files, the way UBT will | clean |
| `-Wshadow` across all of `Sim/` | zero |
| Non-ASCII in any user-facing string | none (comments only, as always) |
| New `UENUM`s are `uint8`, new `USTRUCT`s declared before first use | yes |
| Duplicate `UFUNCTION` names in a class | none |
| Any new Blueprint verb left private by accident | none; all public |
| Thirteen preflight checkers | pass |

What could **not** be checked, and is what §1 is looking for: UHT itself,
MSVC, and anything about how a `UPROPERTY` behaves at runtime.

Roughly **3½ hours**, of which §11 is two of them and is still the point.

---

## 1. Get the build green · ~15 min

1. `git pull` on `claude/dirtbag-unreal-port-ggvybe`.
2. Right-click `DirtbagUE.uproject` → Generate Visual Studio project files.
   **Do this rather than skipping it** — three new bridge files
   (`SimTrad.cpp`, `SimHabits.cpp`, `SimNarrator.cpp`) have appeared and the
   project files will not know about them otherwise.
3. Build. **Four commits of container-green C++ land at once**, including
   three new sim modules and four new Blueprint types (`FDirtbagRack`,
   `FDirtbagLogbook`, `FDirtbagQuirks`, `FDirtbagBeat`) plus four new
   enums. **If it fails, paste me the full log before touching anything** —
   do not start fixing signatures by hand, that is what the round trip is
   for.

4. **Load your save.** It is **v34** and migrates from anything back to v1.
   Two migrations run: v32→v33 gives you an empty harness (there was no trad
   to place gear on), and v33→v34 gives you an empty logbook and no quirks.
   **Both are meant to load as nothing** — a career that climbed before
   anybody was counting has nothing counted.

---

## 2. Two new keys, and one new question

- **G at the gear shop** — buys the next rung of the rack: nuts $190, then
  cams $640, then doubles $1,150. The shop prompt says which and what it
  costs. **The first one opens a crag**; the rest are upgrades.
- **The creation flow now asks five questions, not four.** After "what are
  you like?" comes *"One more thing about you."* — six pickable quirks, keys
  1–6. This only shows on a **new career**, so to see it you need a
  handover or a fresh save.

Nothing else moved. Every other key is where it was.

---

## 3. Trad, and the crag you cannot go to without buying something · ~25 min

`Discipline` is `{Boulder, Sport, Trad}` now. The design is that **it is not
a second climbing model** — it is the same runout model reading gear you
placed instead of bolts somebody drilled.

**The Old Buttress** is the third crag: an hour up the hill, east-facing,
sixteen lines and three unclimbed. Its classics are *moderate* where the
cave's are hard, which is deliberate — trad is the one discipline whose
entry-level lines are the famous ones.

What to look at, in order:

1. **Try to go there without a rack.** You should not be able to lead
   anything. That gate is the reason the rack is a purchase and not a
   pickup.
2. **Buy the nuts, lead something moderate.** Watch the leader stop and
   place. Watch where they *don't* — a sensible leader climbs past rubbish
   rock rather than spending a piece on it.
3. **Then lead the same grade on the cave's bolts.** Measured, trad comes
   out about a grade harder. **The question I need answered is whether it
   *feels* like a grade harder or like the game being mean**, because those
   are the same numbers and different games.
4. **Get on *Ropeless in a Sense*.** It is a face route on a trad crag on
   purpose: it goes at a grade you can pull and there is almost nothing to
   put in. Compare it with **Bombproof**, same idea from the other end.
5. **Upgrade to cams if you can afford them.** Measured over thirty years, a
   career can only reach the top of that shelf if it decides to *work* for
   it — a dirtbag's balance sits between $125 and $290 for its whole life.
   **If you find yourself able to buy doubles casually, tell me** — the
   prices are wrong and I will re-measure.

---

## 4. What you have turned into · ~10 min, mostly passive

Two seasons of climbing the same way and it stops being a thing you do and
becomes a thing you are. **A habit can be stopped; a quirk cannot.**

- **Bottom-left, under the standing line**: what you have been doing lately
  (in a live colour) and what you have become (dim). **Both are blank most
  of the time and that is correct** — if either says something on a fresh
  career, that is a bug and I want to know.
- **Overnight, in the slow news slot**: *"Somewhere in the last couple of
  seasons you became obsessive."* Same colour and size as a talent
  surfacing, deliberately — both are things you found out about yourself.
- **At the handover**, under the epitaph: the long form. A list of ascents
  says what a career did; that line says who was doing it.

Measured, a thirty-year career earns **one to four** of these, and *which*
ones depends on how you played: a gym member ends up a plastic merchant and
a morning person, a sporadic outdoor climber gets one or two, and **only a
trad leader ever becomes bold.**

**The thing to watch for**: a habit you have stopped doing should disappear
from the corner within a season or so. If it sticks, the decay is broken.

---

## 5. The wall talks now · ~10 min, and this is the one I most want your eye on

Every attempt in the game produces beats — *"Clipped. The ground stops being
the question now."*, *"The forearms are going."*, *"A long way above a piece
you do not believe in."*, *"Off pulling up the rope. That is the classic and
it never stops being infuriating."*

**The design is that it knows when to shut up.** A line a move is a log, not
commentary. It speaks only when something beats everything before it — so a
route you fought for the whole way gets *one* "that should not have stayed
on", at the worst move, not sixteen.

Three surfaces, and they should never say the same thing twice:

- **During a go**: a toast, coloured by kind, held longer the louder the
  beat is.
- **After a go**: one sentence on the session panel, above the route name,
  and it stays until you pull on again — *"Off at the crux. You had been a
  long way above the gear for a while."*
- **The count** (`move 9 of 12, skin 4.2`) is still there and is now the
  *only* thing that toast says. The sentence used to lead it and now the
  narrator says it twice already.

Two things I changed on the wall while I was in there, both of which you
should sanity-check:

- **`"shake  -14 pump"` is gone.** It had been there since Phase 0 and it is
  a stat line where a sentence belongs. A shake now says whether it was a
  chalk-up or the rest that gave you the route back. **The "nothing to milk
  here" line stays** — a shake that *cost* you is genuinely invisible to the
  timeline, so the wall still has to say that one itself.
- **The end-of-attempt toast lost its sentence** for the reason above.

**What I need from you**: does the wall talk too much? I tuned the
thresholds against measured attempts, never against watching one. If it is
chatty, the numbers to move are in `NarratorDials` and I would rather raise
them than have you stop reading it.

---

## 6. The indoor career now exists · ~30 min to see the bottom of it

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

## 7. Being hurt is no longer a wait · ~20 min, and this is the best thing to report on

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

## 8. And three things that are wrong with you that are not the injury · ~10 min

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

## 9. Still outstanding · the camera · ~15 min, needs your eye

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

## 10. Still outstanding · sound · optional

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

## 11. The Lot blockout · ~2 hours · **still the whole point**

Unchanged from the last list, because nothing I have built since could touch
it. Repeated in full because it is the item that matters.

You said: *"i dont even know what assets to use or how to put things
together."* The answer to the first half:

> **For the blockout, none. Do not buy anything yet.**

The shopping list is written down (`notes/phase6-the-place.md` Part 1) and
**none of it is needed to build the Lot.** Buying before the blockout is how
you end up dressing a shape that turns out to be wrong.

Why the Lot first: **you sleep there every night of a thirty-year career.**

### 11a. Make ground · ~20 min
1. New Landscape actor. Sculpt roughly — **you are making occlusion, not
   terrain.** A hill between the Lot and the road is the entire job, because
   what you cannot see is what makes two places two places.
2. Flat pad for the Lot, big enough that crossing it takes a few seconds.
3. **Do not texture it.** Grey is correct.

### 11b. Move what you have onto it · ~30 min
The trigger volumes get **relocated**, at real distance from each other:
the van (sleep, and the hangboard on its side door, and now prehab), the
fire, the dog bowl, the travel spot at the edge, a rest spot by the pads.

**The distances are the design.** If the fire is two steps from the van,
sitting down at it costs nothing and the evening stops being a choice.

### 11c. Play it grey · ~20 min
Walk it. Sleep. Drive to Roadside. Climb. Come back.

**It will be grey and it will be a game.** That is the checkpoint. If it
feels wrong grey, no amount of Megascans fixes it and the layout needs
another pass.

### 11d. Only then, dress one pocket
One at a time, playing after each. The climb walls are last: a wall is a
spline plus a mesh and **the rock behind it is scenery**, so the rock
purchase blocks nothing. You could build the whole valley and play a season
before buying a cliff.

**The one thing expected to be genuinely hard** is making a hand-drawn hold
spline line up with real rock features. **Do one wall and find out how much
fuss it is before committing to twenty-five.**

---


## 12. Then play — and these are the questions

Ranked by how much I need the answer:

1. **Does the grey Lot feel like a place?** §11c. If yes, the rest of
   Phase 6 is work rather than risk. Everything else on this list is a dial.
2. **Does the wall talk too much?** §5. Measured against attempts, never
   against watching one.
3. **Does a trad lead feel a grade harder, or feel mean?** §3. Same numbers,
   different games.
4. **Can you ever afford a scan, or a rack of cams?** §7 and §3. Both systems
   lean on cash-on-hand being the binding constraint, and the probe says a
   career sits between $125 and $290 for thirty years. If you are richer
   than that, several prices are wrong.
5. **Does the tooth make you spend?** §8. Meant to be the one thing you
   resent paying for and pay for anyway.
6. **Play one comp at Regional.** §6. Does the semi feel like a second
   chance or like a chore?
7. **Skip a World Cup round you cannot afford.** §6. Does the table moving
   away from you land?
8. **Camera**, §9, with somebody who is not you.

---

## 13. What is deliberately not done

- **Phase 6 is untouched by all of this.** Nine of eleven walkable zones are
  empty, the Lot is trigger volumes, and no amount of sim work changes it.
  **Three phases of sim depth landed in one day and the presentation is
  still one widget and a canvas HUD** — which is the roadmap's own warning
  about this project arriving on schedule.
- **No audio assets.** §10, and now there is a reason to care: `BeatKind`
  exists so a sound can key off a beat rather than off its words. Nothing
  does yet.
- **No camera keys off a beat either.** Same enum, same gap.
- **`Beats()` is a replay door nobody walks through.** The highlight reel is
  a Blueprint job whenever you want it.
- **A watched bot attempt says less than a driven one** — beats fire from
  the live path only. Correct for now; revisit if watching somebody else
  climb becomes a feature rather than a fallback.
- **Phase 7's `social` axis** is still unwired — it needs turnout, which
  needs somewhere for people to turn up to.
- **Personality drift** — Phase 7's third named thing — is not built. The
  axes hold whatever creation set them to.
- **Phase 8's balance call is still open**: 502 races in thirty years is a
  lot. My reading is "fewer races, longer clock", but it is taste and it is
  yours.
- **Phase 11 (a life outside it)** is the last unbuilt phase, and the
  roadmap's own note is that it is the one most improved by Phase 6 landing
  first — a hobby needs somewhere to happen.

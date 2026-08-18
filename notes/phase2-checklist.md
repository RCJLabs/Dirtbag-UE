# Phase 2 — conditions at the desk (Evan)

Sim side is done and green (19,111 checks). This is what conditions need
from the editor, and what to look at to judge whether the mechanic works.

---

## 0. What changed

- **The day now has weather.** Temperature, humidity, cloud and wind,
  generated from your world seed and the day number. It is never saved —
  same seed and same day always gives the same forecast, so a reload cannot
  hand you a different day.
- **Friction is real.** Every attempt used to resolve at friction 0.5
  because that was a default nobody overrode. Attempts now resolve against
  the actual conditions where and when you are climbing.
- **The gym is deliberately exempt.** Indoors is flat and boring, because a
  gym has no shade line. Nothing about your Phase 1 gym changes.
- **Rock temperature, not air temperature.** The wall banks the sun's heat
  and keeps giving it back after the sun has moved off, so a face is still
  warm well into the shade. Waiting for it to cool *is* the mechanic.

---

## 1. Rebuild

1. Close Unreal and Visual Studio.
2. **Fetch origin → Pull.**
3. Delete `C:\Dev\DirtbagUE\Dirtbag-UE\DirtbagUE\Binaries`.
4. Double-click `DirtbagUE.uproject` → **Yes** to rebuild.

The build should be clean. Two whole classes of my earlier breakage are now
caught before they reach you by `tools/check-engine-defs.py`, which I run
before every engine-side commit.

## 2. Nothing to place — check the HUD

Press Play. Under the needs bars you should now see a conditions line. In
the gym it reads:

> `indoors - the holds are exactly as good as they ever are`

That is correct, not a stub. The mechanic is outdoor-only.

## 3. Make an outdoor crag (the actual Phase 2 work)

Conditions only bite outdoors, so this needs a crag. Blockout quality is
fine — this is about whether the *decision* is interesting, not whether the
rock looks like rock.

1. Build a second area away from the gym, or a new level. A few boulders'
   worth of slabs is plenty.
2. Put climb walls on it as in Phase 1 (`Board Index` still chooses the
   problem for now — a real outdoor route table is the next container-side
   bite).
3. Select each wall you want outdoors and set **Details → Dirtbag|Route →
   Venue = `Crag`**. That is the whole switch, and it lives on the wall
   because that is the thing you can actually click.

   > **Correction.** An earlier version of this file told you to set
   > `b Indoors` on the DirtbagGameInstance. You cannot: a game instance is
   > not an actor, it is not in the outliner, and it has no details panel.
   > That instruction was impossible to follow, and the symptom was a crag
   > wall serving *The Blue One V7* off the gym board. Venue now lives on
   > the wall and on travel spots (**Arrive At**), so where the game thinks
   > you are can never disagree with what you are standing in front of.

   **Two fields, both on the wall: `Venue` and `Board Index`.** Venue
   decides which book the index reads from, so changing the index alone on a
   Gym wall just picks a different gym problem — and an index past the
   eight-problem board clamps to the last one, which is how a wall set to 25
   ends up showing *The Blue One V7*.

   Everything under **Advanced** in `Dirtbag|Route` (Route Name, Grade, True
   Grade, World Seed, Route Type) is a fallback for test maps with no game
   instance. With one present they are overwritten at BeginPlay, so what the
   panel shows there is last run's answer rather than a setting to maintain.
   They are collapsed under Advanced for exactly that reason.

   Setting Venue to `Crag` also sets **Crag Aspect** to the crag's own
   (east) on load. Change the aspect afterwards if you want to feel how it
   moves the window — the crag will not fight you, it only refuses to
   disagree with itself.

   **Board Index now means the guidebook.** In book order: 0 Roadside
   Attraction V0 · 2 Second Breakfast V2 (the famous sandbag — says V2,
   climbs V3) · 5 Diesel V5 \*\*\* · 6 Shade Line V4 \*\*\* · 13 Chalk Ghost
   V6 \*\*\* · 20 The Guidebook Lied V7 \*\*\* · 23 Send Train V8 \*\*\* ·
   25–27 the three open projects. Point four walls at 6, 5, 13 and 25 for a
   proper spread.
4. Now the HUD line becomes the real one:

> `greasy - 71F on the rock, 34% humidity.  window 6:30pm to 7:45pm, best at 7:15pm`

**Aspect is the interesting dial.** Try each and watch when the window
lands: east-facing is bimodal (dawn, or evening once it has cooled from the
morning sun), west-facing peaks in the morning before the sun swings on,
south-facing bakes all midday and comes good around 6pm, north-facing never
takes a direct hit and just tracks the air.

## 3b. Place a rest spot (this is the one that makes the window mean anything)

Until now nothing in the game passed time on purpose, so "wait for the
window" was advice with no verb attached. There is one now.

1. **Place Actors** → `Dirtbag Day Spot` → drop it near the boulders. Set
   **Kind = `Rest`**. A log, a rock, or nothing at all — the actor has no
   mesh, so put a cube next to it if you want to find it again.
2. Leave **Wait For Window** ticked. One press then sits *exactly* until the
   rock comes good, rather than making you press it eight more times.
3. Walk up to it. The prompt carries the forecast, because that is the only
   reason anyone sits down:

   > `Sit and wait?  (E)  -  The rock comes good in 95 minutes.`

   and afterwards:

   > `Sat for 1.6 hours.  It is on, right now.`

4. On a day with no window at all it says so plainly — *"Today is not going
   to come good. Go and do something else."* — which is your cue to take a
   shift instead. About a third of days are like that.

Resting buys back a little energy (4 an hour), nowhere near a night's sleep.
It is how you spend hours you cannot climb in, not a way to farm energy.

**The day this unlocks:** wake, drive out, brush the project through the
greasy middle of the day, sit until the shade line arrives, then spend your
skin in the hour that is worth it. That is the loop the whole phase is for,
and it is now playable end to end.

## 4. What to actually judge

The measurements say the decision is live (`notes/phase2-window.md`). What
they cannot tell me is whether it *feels* live. Specifically:

1. **Does waiting feel like a decision or like a chore?** Right now the only
   way to pass time is the Travel spot and shifts. If waiting for 6pm is
   just pressing E on a bed, the mechanic is arithmetic, not gameplay — and
   the fix is that waiting needs somewhere to happen (the Lot, the fire, the
   neighbours). That is the rest of Phase 2, and your answer decides how
   soon it gets built.
2. **Is "no window today" interesting or just annoying?** About a third of
   days never come good. Driving out and finding grease is meant to be a
   real outcome. If it reads as the game wasting your time, the threshold
   moves.
3. **Can you feel the difference between a burn in the window and one
   outside it?** You should. At your limit it is roughly 39% versus 70%.
4. **Does the forecast line tell you what you need?** It is one line and it
   is doing a lot of work. If you find yourself wanting the whole day's
   curve, say so — that is a graph, and it is easy.

---

## 5. What is deliberately not built yet

Named so you know they are decisions, not oversights:

- **Seasons.** One temperature centre stands for the whole year. A summer
  that drives everyone to the north-facing crag at dawn is a later pass; the
  dial is already where it will live.
- **Forecasts that lie.** Weather is exact when you read it. A vague
  multi-day forecast you can misjudge is a better game and a bigger change.
- **Rain.** Cloud blunts the sun; nothing yet stops you climbing or seeps a
  route for two days.
- **Outdoor route content.** ~25 named routes and the FA pipeline (clean →
  work → send → name) are the meat of Phase 2 and are next.

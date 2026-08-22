# The session reads

*2026-08-22. Phase 5, item 3.*

Phase 0's gate:

> **A watcher can tell how close an attempt was without reading a number.**

It was signed off on a blockout wall with a debug HUD — which is to say it
was proved by reading numbers. Re-asking it of a game rather than a
prototype turned up two things, one of them embarrassing.

## 1. Nothing had ever decided what "close" means

The presentation was left to infer it from a pump bar and a move index.
That is the same as not having it: **two attempts that both end at move 9
of 12 can be a heartbreak and a formality**, and the timeline knows which
while the staging does not.

So it is a sim judgement now, with dials and tests, and the staging reads
it — the CLAUDE.md rule is exact about this: *if it can be unit-tested, it
does not belong in the presentation layer.*

`HowClose(result, totalMoves)` → 0..1, from two terms:

**Progress, top-heavy.** `topHeavy = 1.8`, so nine moves of twelve is not
three quarters of a send — it is most of a route and none of a tick, and a
linear reading calls those the same thing. Measured through the test:
halfway up scores under 0.40; eleven of twelve scores over 0.70.

**What you fell off.** `blownIt = 0.35`. Getting eighty percent up and
fluffing a jug is the closest thing to a send there is — you *had* it.
Getting eighty percent up and falling off the crux is a good go, and
everybody in the car park knows the difference. The two now differ by more
than 0.15, which is a margin worth staging rather than a rounding
difference — the magnitude lesson from the campfire, which orderings cannot
see.

`HowCloseText` gives six bands and **not one of them contains a digit** —
the test asserts that, because the gate is about a watcher rather than a
reader.

Both verified by reintroduction: flattening `topHeavy` to 1.0 fails the
top-weighting check; setting `blownIt` to 0 fails the two that say a
fluffed jug beats a cruxed move.

## 2. The camera was a tripod

`SessionCamera` was created at a fixed relative offset in the constructor
and **never touched again** — a locked-off shot for the whole attempt. On
any line taller than the frame the climber simply left it, and the watcher
spent the crux looking at rock.

### The rule, learned the hard way

The facing bug cost two builds because a fixed angle was computed from an
assumed level layout. **The container cannot see the level, so it must not
guess at it.** Nothing here invents an angle:

- **The camera as Evan placed it is the wide shot.** Its authored offset
  gives both the distance and the direction out from the wall.
- Everything below only moves *in* from there, so the defaults leave a
  well-framed wall alone.
- `bCameraFollows = false` puts the tripod back exactly as it was.

### What the shot says

**It follows.** Level with the climber, `Distance` out along the authored
direction, aiming a little above their hands so the frame carries the rock
they are going *to* rather than the rock they have done.

**It tightens on a hard move.** `Tension = 1 - odds`, eased slower than the
follow so the framing settles into a crux rather than snapping to it. At
full tension the shot comes in 32% and the lens narrows 7°. Small on
purpose: past about ten degrees it reads as a zoom rather than as tension.

**It breathes with the pump.** Scaled by the *square* of pump, so it is
invisible for the first half of a route and unmistakable at the top — which
is also how being pumped works. Two frequencies that do not divide into
each other, because a clean sine reads as a machine rather than as somebody
holding on. This is the pump bar said without a bar.

Everything reads off the same `FDirtbagSessionReadout` the HUD reads, so
the shot and the bars can never disagree about how hard this move is — and
**a watched attempt is framed exactly like a driven one for free**, because
the readout already answers for both.

### Three bugs I wrote and caught before they shipped

**The lead cancelled itself.** The first version raised the camera *and*
aimed it above the climber, which nets out to a level shot with extra
steps. The height lead is done by aiming only.

**The sway was a drift.** Smoothing toward the camera's own transform,
which already carried last frame's sway, integrates the sway instead of
oscillating it. The shot keeps its own smoothed value now (`ShotLocal`) and
the sway is added after.

**The sway used the actor's Y axis** rather than the shot's lateral, so on
any wall whose camera is not placed along Y it would have pushed the camera
into and out of the rock. It is `Cross(Up, Out)` now — the same assumption
about somebody else's level that cost two builds on the facing, caught this
time before the build.

And one hazard worth naming because it is invisible: the authored transform
is captured **once at BeginPlay and never re-read**. The camera moves during
a session, so reading it a second time would feed this actor its own output
back as the author's intent, and the framing would be gone by the second
attempt.

## 3. The report leads with the sentence

*"Off at move 9 of 12. Skin left: 3.2"* was a number doing the job the
staging is supposed to do. It now reads:

> **You had it up there.**   (move 9 of 12, skin 3.2)

The count stays — a player who wants it should have it — but it stops being
the first thing said, and what leads is the sim's judgement rather than
arithmetic the reader has to do. A near miss reads warm and a nothing go
reads grey, so the colour carries it for anybody not reading at all.

The HUD's `next move 62%` stays too. The gate is *"without reading a
number"*, not *"with no numbers on screen"* — a player driving an attempt
is deciding how hard to pull and deserves the odds. The point is that the
shot now carries it as well.

## What is NOT done

**The gate is not passed and I cannot pass it from here.** Every number
above — 0.32 tighten, 7° narrow, 55cm lead, 7cm sway, the two ease rates —
is a first guess by somebody who has never seen the shot. They are all
`EditAnywhere` under **Dirtbag|Shot** for exactly that reason.

The mechanism is built, preflight-clean and derived from the authored
camera rather than from an assumption. The *feel* is a desk job.

## At the desk

1. Walk up to any wall. The approach shot should be **unchanged** — the
   camera is left alone entirely while idle.
2. Press E on a tall line. The climber should stay in frame to the top,
   with rock visible above their hands. If they sit too high or too low in
   frame, that is **CameraLead**.
3. Watch a crux. The shot should come in and settle rather than snap. Too
   much: **CameraTightenBy**. Too fast: **TensionEase**.
4. Watch a long pumpy route to the top. The camera should be locked early
   and not quite still late. Nothing visible: raise **PumpSway**. Seasick:
   lower it.
5. If any of it is worse than the tripod, **bCameraFollows = false** puts
   the old shot back with no rebuild — it is EditAnywhere.

Then the gate itself, and it needs somebody who is not you: **have them
watch an attempt with the HUD ignored and say whether that was close.**
If they can, item 3 is done.

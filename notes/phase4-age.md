# Age: what bends the curve, and what does not

The obvious way to build aging is to subtract numbers from an old climber,
and it is the wrong way — it makes aging a punishment the player watches
happen. What actually ends climbing careers is not being weaker. It is not
being able to *train* as much: the session you could do twice a week at 25
you can do once at 45, and the one that used to cost three days now costs
five.

So age acts mostly through the budget built last time. Training load clears
more slowly, the injury threshold comes down to meet you, and what you have
keeps only as long as you keep asking for it.

## Three mechanisms, and they are genuinely different things

**Decay** — what happens when you stop asking. 1.6 points of power a year
past its peak, 1.1 fingers, 0.9 endurance. A season of climbing gains far
more than this, so decay is something you outrun by turning up, and it only
catches the player who stops.

**The ceiling** — what your body will still hold, however hard you train.
This one had to be added after measuring: without it, a climber who kept
turning up **never declined at all**, pinning at 100 power from 28 to 52,
because training gains at the ceiling (~8 points a year) outrun decay five
to one. Nobody climbs at 52 the way they did at 28, and no amount of turning
up changes that. Decay is what you can do something about; the ceiling is
what you cannot. It only bites a climber who is actually at it — somebody at
60 power in their forties was never being held back by their age.

**The budget** — recovery slows past 30 (down to a floor of 0.45) and the
injury threshold falls with it. This is the one that changes how a fortnight
is planned.

## The asymmetry is the point

Power goes first. Fingers hold on much longer. Endurance later still.
**Technique and head do not decline at all** — a fifty-year-old reads a
sequence better than they ever have, and that is what keeps them climbing
hard long after the campus board has stopped loving them.

A career that keeps training hard, measured end to end:

| age | on a power line | on a technical line | power | fingers | recovery |
|---|---|---|---|---|---|
| 25 | V7.6 | V8.1 | 70 | 74 | 1.00 |
| 28 | **V11.4** | **V12.0** | 100 | 100 | 1.00 |
| 34 | V10.9 | V12.0 | 87 | 100 | 0.93 |
| 40 | V9.9 | V10.8 | 74 | 87 | 0.82 |
| 46 | V8.9 | V9.7 | 60 | 74 | 0.71 |
| 52 | V7.8 | V8.5 | 47 | 60 | 0.60 |

Peak at 28, still V10 at 40, still climbing at 52 — and the technical line
outlasts the power line the whole way down. That gap is the design intent:
age changes *what kind of climber you are*, not merely how good.

One honest limit on that claim: the gap is 0.6 grades at peak and 0.9 at 40,
so it is real but modest. It is a lean, not a transformation.

## No save version

Age is derived from the day counter — `AgeOn(day)` — and never stored. So
adding a whole system cost no migration, and a save can never disagree with
a birthday. Skills were already saved and they are what age moves.

## What the probe showed that the sim was right about

A climber training three days a week sits at load 92–98 permanently past 40
and would be injured constantly. That is not a bug: it is the sim correctly
saying *you cannot train three days a week at 45*. The probe is the naive
one — a real player backs off, and now has a reason to.

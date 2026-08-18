# The prime window, measured

Phase 2's gate: *"the window mechanic demonstrably changes when players
choose to burn attempts."* This is the measurement that shaped the
mechanic, in the order the numbers arrived. Every table here is
reproducible from `Sim/` — the harness tests in `TestWindowChangesWhenYouBurn`
and friends pin the conclusions.

## 1. The friction lever was already strong enough

`SessionDials::frictionWeight = 6.0` moves effective ability by 0.6 grades
across the full 0→1 friction range. That sounds negligible, and reading it
as negligible was wrong: near the odds cliff, 0.6 grades is enormous.

V5 climber, single burn, fully warm, no beta:

| route | fric 0.00 | 0.25 | 0.50 | 0.75 | 1.00 | swing |
|---|---|---|---|---|---|---|
| V4 | 91.9% | 93.8% | 95.3% | 96.2% | 96.7% | +4.8 |
| **V5** | **38.9%** | 46.8% | 55.1% | 63.5% | **69.5%** | **+30.6** |
| **V6** | **0.4%** | 1.1% | 2.8% | 6.4% | **11.4%** | **+11.1** |
| V7 | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | +0.0 |

Conditions are nearly irrelevant on a warmup and decide everything at your
limit — which is exactly the shape the mechanic wants. **No change to
`frictionWeight` was needed.**

## 2. The window's length is the whole design

Skin, not the clock, ends a session: 8–12 burns, which is 2–3 hours of
climbing. So a window longer than about two hours lets a whole day's skin
fit inside it, and waiting for it costs nothing. That is the same trap that
killed the Phase 1 day clock (skin ended sessions at burn 8 while the clock
read 9am), and it deletes the projecting loop outright:

V5 climber on a limit project, whole day available:

| strategy | V5 project | V6 project |
|---|---|---|
| burn everything early, poor conditions | 74.1% | 25.0% |
| wait for the window, no recon (**8-burn window**) | **98.0%** | **77.0%** |
| wait for the window, no recon (4-burn window) | 56.6% | 13.6% |
| 4 recon burns, then a 4-burn window | 95.0% | 60.1% |

With a window big enough to hold a day's skin, "show up and wait" sends a
limit project 98% of the time knowing nothing about it. Beta, projecting,
and the ledger all stop mattering. **The window has to be about an hour.**

## 3. Getting an interior peak (three failed models)

The window must also land *in the middle of the day*, or "prime" collapses
into "whenever is coldest", which is always a boundary.

1. **Air temperature alone** → 50% of days peaked at 8pm (last light).
   Cooling monotonically improved friction, so the latest legal hour always
   won.
2. **Rock temperature with thermal lag** (the wall banks the sun's heat and
   sheds it slowly — the real reason you wait after the shade line passes).
   Better, and physically right, but with a wide temperature tolerance the
   peak still pinned to a boundary: ~50% at first light.
3. **Narrow temperature band + a season the day crosses.** `tempToleranceF`
   10 and an average day running 37°F at dawn to 63°F mid-afternoon, against
   an ideal of 50°F. Now the rock passes *through* condition on the way up
   and again on the way down, and the aspect decides which crossing is the
   good one. Dawn-pinning fell to 12%.

The window is defined **peak-relative** (`windowBand`), not by an absolute
threshold, so it stays tight on a north-facing wall that is merely good all
day. An absolute floor (`primeThreshold`) still decides whether a day has a
window at all — about a third do not, which is content, not a gap.

Resulting season (seed `crag-1`, 600 days):

| aspect | days with a window | median length | burns it holds |
|---|---|---|---|
| north-facing | 69% | 1.25h | 5 |
| east-facing | 68% | 0.75h | 3 |
| south-facing | 69% | 1.00h | 4 |
| west-facing | 69% | 1.25h | 5 |

Aspect genuinely decides *when*, and it does it correctly: east-facing is
bimodal (dawn, or evening once it has cooled from the morning sun),
west-facing peaks in the morning before the sun swings on, south-facing
bakes all midday and comes good at 6pm, north-facing never takes a direct
hit and tracks the air.

## 4. The gate, passed

Real generated weather, real window lengths, real off-window friction.
V5 climber, recon burns taken two hours before the window opens:

| recon burns | east V5 | east V6 | north V5 | north V6 |
|---|---|---|---|---|
| 0 | 27.4% | 14.1% | 55.5% | 28.0% |
| 3 | 56.9% | 30.0% | 65.8% | 43.9% |
| **4** | 60.3% | 34.4% | **65.8%** | **44.2%** |
| **6** | **62.5%** | **38.4%** | 63.6% | 39.7% |
| 8 | 59.9% | 33.4% | 60.0% | 31.2% |
| 12 (skin gone before it opens) | 59.8% | 31.5% | 60.0% | 29.3% |

Every column has an interior optimum. Burns outside the window buy the beta
you need; too many and the skin you needed is gone before conditions arrive.
The crag's aspect moves the optimum (6 recon on the east face, 4 on the
north) and changes how badly ignorance is punished (27.4% vs 55.5% at zero
recon), so *which* crag is a decision too.

## What is deliberately not modelled yet

- **Seasons.** One `baseTempF` stands for the whole year. A real season
  arc — summer driving everyone to the north-facing crag at dawn — is a
  later pass, and the dial is already the place it will live.
- **Forecasts being wrong.** Weather is generated per day and is exact when
  read. A vague multi-day forecast that can lie is a better game and a
  bigger change; it belongs with the crag-choice loop, not before it.
- **Rain.** Cloud blunts the sun; nothing yet stops you climbing outright or
  seeps a route for two days.

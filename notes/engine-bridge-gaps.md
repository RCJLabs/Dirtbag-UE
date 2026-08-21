# The engine bridge is thinner than the sim

**2026-08-21.** Nine times a function had been written, tested, and called by
nothing. After the ninth (`CoversShoes`) it stopped being worth finding one
at a time, so `tools/check-unwired.py` now asks the question for every public
sim declaration at once: **is this reachable from the engine?**

It found **thirteen more**, and they are not small.

## Why "reachable from the engine" and not "called by anything"

`FactionDay` — the standing drift and the crag-closure countdown — was called
by the season probe *every simulated day* and by the engine never. So the
mechanic measured as working, in detail, for months, while in the actual game
a crag pulled access once and never gave it back.

A harness calling something is not the same as the game calling it. The
checker seeds its roots from `Source/` only, and propagates transitively
through the sim — the probe is deliberately not a root.

## The debt, as it stands

Each of these carries an `unwired-ok: NOT WIRED — …` line at its
declaration, so `check-unwired.py` prints them as debt on every preflight and
the count cannot quietly grow.

**Money the game does not take or give**

| what | consequence in the actual game |
|---|---|
| `FuelFor` | **driving is free in the game.** The probe has always charged it (`season.cpp`), which is where the $918–$1,224 a season came from; the engine never did |
| `WorkSalariedDay`, `SalaryDayPay`, `SalaryOwnsHour` | **the salaried-job trap does not exist.** The engine can ask `SalariedToday()` and has no way to work the day |
| `MonthlyStipend` | **a sponsored player is never paid.** The $640/month title tier pays $0 |
| `ReviewSeason` | **a deal is never reviewed**, so no rung is ever won or lost |

**Things other people are supposed to be worth**

| what | consequence |
|---|---|
| `WillBelay`, `BurnsTheyWillHold`, `BestBelayer`, `BelayText` | **"no partner, no pitch" is unenforceable.** `NeedsABelayer` is wired, so the game can ask the question and cannot answer it |
| `PsycheFrom` | a partner lifts no psyche |
| `BetaMultiplierFor` | standing buys no beta |

**And one that is worse than unwired**

`ShoePenalty` is not called by anything — because the resolver prices dead
rubber with **its own inline copy of the same formula**. Two copies of one
question, which is precisely the drift this project has written three
separate rules against. Whichever one is right, there should be one.

## What this does and does not change

**It does not touch any measurement.** Every probe result in these notes —
the Phase 3 gate, the skin sweep, the kit, the body, sponsorship — runs
against the sim directly, where all of this code does execute. Those numbers
stand.

**It changes what "built" means.** Phase 3 and Phase 4 are recorded as having
their named scope complete, and the sim's half genuinely is. The engine's
half is not, and nothing said so, because a `BlueprintCallable` that exists
is indistinguishable from one that is called.

## Wired since (2026-08-21)

**`FuelFor`** — charged in `DriveVan`, which every drive in the game comes
through. Charged rather than refused: you cannot decline to have burned the
fuel you already burned, so a skint player arrives owing for the drive.
`LastDriveFuel` carries the number to the arrival toast, because a cost the
player never sees reads as a bug.

**`WorkSalariedDay` / `SalaryDayPay`** — run from `Sleep`, right after
`SleepToNextDay` resets the day to the wake hour. At dawn, not offered as an
action: a trap you can decline is not a trap. `SalaryOwnsHour` stays unwired
and now says why — with the day worked at dawn the clock is always past the
shift, so it is unreachable by construction rather than by omission.

**`MonthlyStipend` / `ReviewSeason`** — monthly and yearly, from `Sleep`.
The review needed `daysHurtThisSeason`, which had to become **saved state**
(SAVE_VERSION 15), because being hurt pauses the review clock and a reload
must not launder a season spent injured into a season spent slacking.

> Worth recording: **`ReviewSeason` was unwired in the probe too.** Every
> other gap here was engine-only. This one meant no career anywhere — played
> or simulated — had ever been reviewed, so the "a failed review costs a rung
> rather than the career" line in the sponsorship changelog was describing
> code that had never run outside a unit test.

**`PsycheFrom` / `BetaMultiplierFor`** — the Lot is worth something now.
Standing scales what a partner spells out for you (a new `generosity`
argument on `ShareBeta`, which scales the share and never the ceiling), and
sitting at the fire lifts psyche by **the best of the people there, not the
sum** — summing would make crowding the fire a strategy, and an evening is
lifted by the person who lifts it, not by a headcount. `FactionOf` came back
with them: it was only ever unreachable because its one caller was.

## A hole in the tooling, found by trying to break it

`check-mirror-coverage.py` checks that every sim field is read by its
`FromSim`. It does **not** check `ToSim`, and deleting
`Out.daysHurtThisSeason = In.DaysHurtThisSeason` from the engine-to-sim
direction passes every checker and every test in the repo.

That direction is not cosmetic. The sim increments this field inside
`SleepToNextDay`, and the engine reaches that through a ToSim/FromSim round
trip — so a mirror that dropped it on the way *in* would throw the increment
away every single night and the injury pause would never once fire. Exactly
the class of silent bug this project keeps finding, one layer down.

**Fixed, and it caught an eleventh on its first run.** The symmetric check
turned out to flag only three fields, not the flood I expected — and one of
them was real:

**`SessionState::shoeWear` never survived the bridge.** `StartGymSession`
copies the rubber off the career into the session, and the very next
`ToSim(Day)` — of which the engine does a dozen a day — put it back to 0.0.
So every attempt in the game resolved with **brand-new shoes**, and the
whole gear-wear economy, resoles and all, cost the player nothing at the
wall.

The old `mirror-skip` on it read: *"Blueprint reads it from
Player.Shoes.Wear."* That was true and it was the wrong question. A skipped
field is not merely absent from Blueprint — on a struct the engine
round-trips, it is **erased from the sim on the next call**. Worth keeping
as the shape of the mistake: a justification that answers *does anyone
display this* when the question was *does anyone lose this*.

**`ShoePenalty`** — see `notes/shoes.md`. The unwired one was a symptom: the
resolver carried its own copy of the formula. `ShoePenaltyFor` is now the
single pricing of dead rubber, the golden vectors did not move (which is what
says the two copies really were identical), and a test pins all three
declared dial mirrors — shoes, the runout, and the injury penalty — because a
comment saying "mirrors GearDials" is not a guard.

Measured before calling it fixed, since until today nobody had ever felt it:
a climber who never replaces their shoes sends **1.0 against 3.7** on the
same burns, winning 7 seeds, tying 3, losing 0 — for about **$9** of the
year's cash. The gear shop now says what the rubber is costing you.

## Still open: the belayer, and only the belayer

`WillBelay`, `BurnsTheyWillHold`, `BestBelayer`, `BelayText` — the Shaded
Cave's mechanic, and held deliberately until the cave is placed in the level.
`NeedsABelayer` is wired, so the game can ask the question and cannot answer
it; wiring a belayer picker with nowhere to belay would be guessing at a UI.

## The rule this leaves behind

The old rule was *anything that happens overnight happens in `Sleep`*, which
came from four ticks that were never called. It was the right rule and it was
too narrow: it only covered per-day ticks, and eight of the thirteen above
are not ticks.

The rule now is the checker. A public sim function is either reachable from
`Source/`, or it says in one line why not.

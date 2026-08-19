# Playing a season headless

`Sim/tools/build-season.sh [days] [seed] [restUntilSkin] [quiet]`

Phase 2's last gate is *"a full season plays start to finish"*, and the way
to test that without a person at the keyboard is to actually play it: a
policy that wakes, reads the forecast, chooses between the crag, a shift and
the fire, waits for the window, spends skin, feeds the dog, sleeps, repeat.
Every call it makes is the same function the game makes.

It is a **probe, not a test** — it explores where the harness asserts. It is
not in `run-tests.sh` for that reason.

---

## What it found

### 1. Flailing was the fastest way to get strong — fixed

Training read the grade on the tag and nothing else. `challenge` came from
the route's true grade against your skill; `result.highpoint` was never
consulted. So falling off move one of something impossible trained exactly
as well as nearly doing it.

The probe found this by accident: its first policy always picked the hardest
line available, and reached **V8.7 in a year without a single send**.

Fixed by scaling the targeted skills by how far up you actually got, with a
floor (`engagementFloor`, 0.2) for what pulling on teaches regardless. The
same year now runs V5.0 → V5.9, and `TestFlailingIsNotTraining` pins it.

### 2. The warmth trap — real, unfixed

Warmth is earned per move climbed, so **a line you cannot start is a line
you can never warm up on**, and being cold makes it harder still. The
probe's first policy averaged warmth 0.33 and 4% odds on the first move,
across 519 burns on one line.

This is arguably correct — it is why climbers warm up on easy problems — but
nothing in the game says so, and a player can grind a season without the
feedback that they are in a hole. The 2D game's answer is warming up on
moderates first. Worth a nudge in the UI rather than a mechanical change.

### 3. Rest days do not reliably help — real, unfixed

Skin is a conserved resource: ~1.5 regrows a night, a burn costs ~1, so a
year is about 550 burns however you spread them. Climbing every day gives
many thin-skinned sessions; resting gives few fresh ones. Total burns barely
move.

**A caution about this one.** On seed `crag-1` resting to skin 2–6 produced
**24 sends against the grinder's 8**, which looked like a clean result and
nearly went into this document as one. Across five seeds it evaporates:

| rest until skin | crag-1 | crag-2 | crag-3 | crag-4 | crag-5 |
|---|---|---|---|---|---|
| 0 (climb daily) | 8 | 7 | 7 | 6 | 8 |
| 2 | 24 | 7 | 8 | 5 | 6 |
| 4 | 24 | 7 | 8 | 5 | — |

`crag-1` is an outlier, not a trend. The honest finding is that **freshness
currently buys you almost nothing**, which is a design question worth
answering deliberately: thin skin costs ability and warmth costs ability, so
a concentrated session *should* be worth more per burn than a scattered one,
and measurably is not.

### 4. A year is about 6–8 sends

Consistent across seeds and policies, with a policy that only ever tries the
hardest thing it has not done. That may be right for at-limit-only climbing
— a real player also laps moderates for mileage, which this policy never
does — but it is the number to look at first if a career ever feels thin.

---

## What it confirmed

- A year runs start to finish with no assertion failures, no drift, and a
  clean save round-trip at the end.
- Money holds: cash never went below $93 across a year, no broke days, no
  hungry nights. Bills and shifts are in balance at this scale.
- The Lot takes the open lines on schedule and the player can still get one.
- The dog gets adopted and stays fed on about $1.50 a day.

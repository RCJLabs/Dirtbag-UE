# The Dirtbag Year

**2026-08-21.** First of the four unported systems from
`notes/what-the-2d-game-still-has.md`. Built on the obvious reading of
`concepts/DIRTBAG.md` §4's *"salaried-job trap + Dirtbag Year achievement"*,
because the repo carries the trap and not its counterweight.

## The rule

**365 days in a row without taking the job that owns your hours.**

- **Odd jobs do not break it**, and must not. The board is how a dirtbag
  eats, and a year spent hauling trail and setting at the gym is the most
  dirtbag year there is. What breaks it is signing for the nine-to-five.
- **It breaks at the signature, not the first shift.** You knew what you were
  doing when you shook hands.
- **A day you hold the job is not a day of the streak**, weekend or not. The
  version that only skipped *working* days would take five years to earn one,
  because the salary owns Saturday too. There is a test for this.
- **Banked years survive.** The job takes the year you were in the middle of,
  not the ones you finished. `longestStreak` survives too.
- **365 rather than a season**, because the point is that you got through a
  *winter* on it. A season-length version is handed to anyone who arrives in
  spring and leaves when the money runs out, which is a holiday.

## Why it exists

The trap was built and its counterweight was not. `WorkSalariedDay` costs you
the hours the day was for and pays in money — and until now **the only thing
refusing it bought was the absence of a cost**, which is not something a
player can feel. A trap needs a thing you were choosing instead.

Measured, thirty years:

| | years banked | longest run |
|---|---|---|
| never signs | **30** | 10,950 days |
| signs once, for a week, in year five | **29** | 9,150 days |

And in the career line, which is where a career is actually read:

> 23 seasons. Hardest: The Guidebook Lied, V7. 2 lines that are yours now.
> **22 Dirtbag Years.** the sit start to Shade Line never went, after 44
> tries. Retired at 46, with the gate still shut.

against the same length of career that took the salary:

> 23 seasons. Hardest: Send Train, V8. One line that is yours now. the arete
> left of Diesel never went, after 44 tries. Retired at 46, with the crag
> open.

**Nothing.** That is the counterweight: the career that signed has a harder
send and no answer to what it did with thirty years.

## Where it surfaces

- `DirtbagYearLine()` — bottom-left with the slow numbers, silent until the
  first whole year. It moves once a year; it is the slowest number on screen.
- `DirtbagYearNews` — said once, on the morning one lands, up where news
  goes. *"A year today, and nobody has owned an hour of it."*
- `TakeSalariedJob()` now **returns the days it cost**, so the moment of
  signing can be honest rather than leaving it to a summary nobody reads.
- `LegacyText` — before the nemesis and the retirement, because this part of
  the record was a *choice* rather than a result.

## Save

`kSaveVersion` 15 → 16, `MigrateV15ToV16`. Old saves start their first streak
at zero. There is no honest way to reconstruct one — a climber who has never
been salaried might be four hundred days in and the file does not say — and
awarding a year that was never lived would put a line in somebody's legacy
that never happened. **Losing history beats inventing it.**

## A test crashed the harness, and the fixture was why

Bumping the save version made `TestSponsorSeasonReview` throw
`std::out_of_range` out of the whole run. Its fixture derived itself from the
build's own writer — the right instinct, and the comment says so — but then
hard-coded `"version=15\n"`. On the bump `find` returned `npos`, `CHECK`
recorded the failure without aborting, and `replace(npos, ...)` threw.

Now `DropSaveLine` and `SetSaveVersion` derive the version, and every
migration fixture uses them. A save bump is a one-line change and never a
crash.

## What I still need from Evan

The rule above is my reading of five words. Two things the 2D game may
answer differently:

1. **Is it a one-time achievement or a counter?** As built, a career that
   never signs banks **one per year — thirty of them over thirty years**,
   which reads more like "never had a boss" than like an award. If the 2D
   version is a single badge you either have or do not, that is a two-line
   change and probably the better design.
2. **Does it *do* anything?** Right now it is recognition only: a line on the
   HUD and a line in the legacy. That is defensible — the pillar is *"the
   life, not the score"* — but if the 2D game attaches standing, psyche, or a
   sponsor's interest to it, say so and it is a small addition.

Neither blocks anything. Both are corrections rather than rewrites.

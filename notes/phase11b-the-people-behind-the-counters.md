# Phase 11, pass 2 — the people behind the counters

*2026-08-24*

Phase 11's third gate: **somebody in the world greets you by what you did
last time.** It is the only part of the bucket that is not content — fishing,
the garden, the caravan are all another row on `Thread`, and this is a
different shape: *a person who holds exactly one fact about you and hands it
back the next time you are in front of them.*

Two rules, and the file is entirely downstream of them:

> **They hold one thing, and it is the loudest thing they have heard since
> they last saw you.**
> **They say it once. Then they go back to nodding.**

The first is what keeps it from being a feed. A shopkeeper who lists your
season at you is a changelog with a face; one who says *"heard you got the
Prow"* and nothing else is a person. The second is what keeps it from being
wallpaper — a line that repeats every visit stops being a greeting inside a
week, which is the same rule the injury line, the habit label and the life
label already follow.

## The loudness is the enum

```cpp
enum class Heard {
  None = 0,
  Away,       // nobody tells them this one
  Broke,      // and only they saw this one
  Hurt,
  Sent,
  Named,
  Won,
  Sponsored,
};
```

The ordering *is* the rule about what gets talked about: a new memory
replaces the held one only if it compares greater, so putting a fact in the
right place in this list is the whole of adding one. There is no second
table to disagree with it.

Two entries are deliberately odd, and both earn it:

- **`Away` is derived, not told.** It is what is left when it has been a
  season and there is nothing else — the only memory in the file that fires
  for doing nothing at all.
- **`Broke` is local.** The bill you could not cover is between you and
  whoever was standing there. Everything else is a valley, and valleys talk.

**Equal replaces.** Two sends in a fortnight and what they bring up is the
second one, because the question a regular is answering is literally *what
did you do last time*. Only a quieter fact is dropped — and dropped, not
queued, because a person is not a mailbox.

## Familiarity keeps a floor — on purpose, and unlike a Thread

`Local::known` fades when you stop turning up and floors at `everKnew ×
knownKeeps`, exactly like `PartnerBond::rapport` and exactly unlike
`Thread::warmth`, which has no floor at all. All three are right:

| | ages toward | why |
|---|---|---|
| `rapport` | a fraction of the best it was | they still know what they know |
| `known` | a fraction of the best it was | you stop being current, not a stranger |
| `warmth` | **zero** | the whole point is that it can be lost |

That is the third time this project has needed *a value that ages needs a
memory, not just a slope* — the ranking's rolling window, the scars' fade,
rapport — and **the first time it was built that way rather than found that
way.**

## Where each fact comes from

Five of the seven have an obvious single site, and the sixth is derived:

| fact | told at |
|---|---|
| `Sent` | `ApplyAttemptToDay` — the one function every burn passes through |
| `Named` | `CreditFirstAscent` — the reason naming a line is a verb |
| `Hurt` | the night's roll, beside `StartComeback` |
| `Broke` | `EatMeal`, to the person standing there |
| `Away` | derived in `WhatTheySay` |
| `Won`, `Sponsored` | **detected in `SleepToNextDay`** |

The last two are the awkward pair and the note explains why they are
awkward: a season title and a signature are **assembled by hand at two call
sites each** — once in the engine and once in the probe — so there is no one
function both consumers pass through except the night tick. Detecting them
there needs a record of what the town already knows, which lives on
`Locals` rather than on the career, because *has this got around yet* is a
fact about the town.

## What the reintroduction pass found

Sixteen defects put back one at a time. The valuable failure was not a
missing rule — it was **a test that crashed instead of failing**.

`TestTheTownIsSeededByTheNight` wrote `At(town, Service::Meal)->name`
directly. Remove the roster's seeding and `At` returns null, the test
segfaults, **the whole binary dies, and every test registered after it never
runs** — so one crash came back as four separate defects "not caught",
including three that had perfectly good coverage. A test that cannot fail is
bad; a test that crashes is worse, because it takes its neighbours with it.
There is now a checked `Counter()` accessor and no raw dereference anywhere
in these tests.

Three real gaps under it, all now covered: the bill leaking to the whole
town through the day loop, the first ascent never reaching the shop, and the
regular's discount never being charged.

And two of my own defect *injections* did not compile — `-Werror` again,
which is why the runner greps for `error:` alongside `FAIL`.

## What the measurement found

Four policies, two seeds, thirty years each.

| | visits | greeted | first | sent | named | sponsored | away |
|---|---|---|---|---|---|---|---|
| greedy | 595 | 40–42 | day 95–259 | 3–4 | 1 | — | 35–37 |
| lifer | ~595 | 44–50 | day 95–225 | 6–13 | 1 | — | 36–37 |
| comper | ~595 | 44–46 | day 72–225 | 8 | 1 | — | 35–37 |
| sponsored | ~595 | 42–44 | day 95–259 | 2–6 | 0–1 | **1** | 35–38 |

Criterion 3 passes: a measured career is greeted by what it did, the
sponsored career gets the line only a sponsored career can get, and roughly
one in fourteen visits produces a sentence — rare enough to be worth
reading.

**And it found a bug that had eaten a career's first two hundred days.**
`Seen` cleared the held memory on *every* visit, including the ones where
they said nothing because you were still a stranger to them. So everything
that happened before you were a regular was silently spent by the burritos
that spent it, and the first version of this table had `named` at **zero in
every career** and *"thought you'd moved on"* as thirty-seven of forty
greetings. It now spends only what was actually said — and, better, decides
that by asking `WhatTheySay` rather than by re-checking `nodsAt`, so the two
cannot drift apart.

**One thing recorded, not fixed.** `away` still dominates, and that is a
fact about the probe rather than about the town: a measured career eats in
town **595 times in thirty years**, one meal every eighteen days, because
the probe's dead days end mid-morning before hunger accumulates and 96% of
its days are dead. Every number in this table is measured on somebody who is
almost never in town. Fixing it means making the probe pass its empty days
properly, which would move every existing measurement in the repo — a
deliberate job, not a side effect of this one.

## The collision

`Sim/DirtbagPartner.cpp` already had a file-local `struct Regular` — the
Lot's roster — and the unity build found the clash in about four seconds.
**Fourth name collision this project has caught that way**, after `Clamp01`,
`Record` and `kColours`, and the only one where the right fix was to rename
the *new* thing: both concepts really were "regulars", so the town's became
**`Local`**, which is the roadmap's own word for them and removes the
ambiguity everywhere rather than hiding it behind a scope.

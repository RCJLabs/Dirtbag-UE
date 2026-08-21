# Dials nobody reads, and mirrors nobody pinned

**2026-08-21.** The reachability sweep asked *does anything call this?* The
same question applies one level down, to the project's first hard
constraint: **dials over magic numbers.** A dial nothing reads is that
constraint quietly inverted — a magic number wearing a dial's clothes,
advertising a tuning that does not exist.

`tools/check-dials.py` asks it of all 234 dials, and enforces two rules.

## Rule 1 — a dial nothing reads

Three, and they were two different mistakes.

**`AgeDials::techniquePeak` / `headPeak`, both 999.0.** Sentinels for a code
path that is *deliberately absent*: `AgeDay` does not touch technique or
head, and says so at the point where it does not. The design is settled and
measured — the asymmetry is the whole finding of `notes/phase4-age.md`.

Deleted rather than wired. A dial whose only value is a sentinel meaning
"this code does not exist" is worse than no dial: somebody eventually moves
it and wonders why nothing happened. If head should ever decline, that is a
design change with a measurement behind it, not a number to twiddle.

**`KitDials::groundedFraction`.** Not a sentinel — a *mirror*, and the
resolver has always read `SessionDials::padGroundedFraction` instead.

## Rule 2 — a mirror nothing pins

Sharing a constant between two dial structs is this project's deliberate
arrangement, not an accident: it keeps `DirtbagSession` from including half
the project to price a move. **Share the function, mirror the constant.**

It is only safe while something holds the copies together. There are five
pairs. Before today, one was pinned.

| pair | what drifting would have done |
|---|---|
| shoes — `GearDials` | the shop quotes a price the wall does not charge |
| runout — `SportDials` | the guidebook describes a runout the resolver does not price |
| injury — `BodyDials` | the physio disagrees with the climbing |
| **pad — `KitDials`** | **retune the pad at the shop, nothing changes at the wall** |
| **`daysPerYear` — `AgeDials` / `ConditionsDials`** | **a birthday and a solstice on different calendars** |

The last one the checker found and I did not know about. Ages would slide
against seasons across a career with no symptom sharp enough to notice until
somebody turned 40 in high summer twice running.

`KitDials::groundedFraction` was renamed to `padGroundedFraction` — matching
its opposite number, so the mirror is visible to the tool *and* to a reader.

## The false negative worth keeping in view

A naive search for `.fieldName` cannot tell `KitDials::noPadGradePenalty`
from `SessionDials::noPadGradePenalty`. The first sweep therefore reported
one dead pad dial and missed the other sitting beside it, because that one
hid behind its own mirror.

So mirrored names are checked by rule 2 and excluded from rule 1 — the only
honest split available without resolving C++ types. It is why the two rules
have to exist together rather than as one.

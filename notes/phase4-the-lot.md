# The Lot left the scale

**2026-08-21.** Evan: *"make the lot less strong so the player wins more."*

The first half was a real bug and is fixed. The second half does not follow
from it, and the measurement says why.

## The Lot was never supposed to be that strong

`Climber(...)` builds a partner as `baseSkill + skillPerDay * ambition *
day`, and the dial's own comment reads:

> *"At 0.03 a season moves somebody about a third of a grade — the same
> glacial pace the player lives at, because a Lot where everyone outpaces
> you is a different and worse game."*

A season is 365 days and a grade is 7.14 skill points, so a third of a grade
a season is **0.0065 a day**. It was **0.03** — **1.53 grades a season**,
four and a half times its own documented intent. Nothing in the repo
disagreed with the comment because nothing checked it.

**And nothing capped it.** Partners are the only climbers in the game with
no age model, so the creep ran forever on a scale documented `0..100`:

| year | Margo | Dev | Trish |
|---|---|---|---|
| 0 | 57.6 | 70.5 | 40.5 |
| 5 | 76.8 | 122.4 | 73.3 |
| 10 | 95.9 | 174.4 | 106.2 |
| 30 | 172.6 | **382.5** | 237.6 |

Trish is the one who is delighted to help and cannot climb your project. At
thirty years she is at 237.

Fixed: the rate is what its sentence says, and `ceiling = 100.0`. Dev now
plateaus around year 17, which is what a strong local does.

| year | Margo | Dev | Trish |
|---|---|---|---|
| 0 | 57.6 | 70.5 | 40.5 |
| 10 | 65.9 | 93.0 | 54.7 |
| 30 | 82.5 | **100.0** | 83.2 |

## But the player does not win more, and the Lot was not why

Ninety years, four generations: **player 1, the Lot 2** — against 1 and 3
before. The player's share did not move; the Lot simply takes fewer, and
the lines it no longer takes are not taken by anybody.

Because the binding constraint is the player's own ceiling:

| | |
|---|---|
| player's peak allround, six careers | **V6.2 – V7.2**, mean **6.6** |
| what the V8 projects need, clean and wired | **skill 65 = V7.1** for 67% |
| the same projects at skill 55 | **1.8%** |

**The player's best year lands just under the bar.** One seed in six crosses
it. That is not the Lot being too strong; that is Roadside's project list
having a hole in it:

| Roadside's open projects | true grade |
|---|---|
| the slab right of the pull-off | V3 |
| the short wall behind the cattle grid | V4 |
| the arete left of Diesel | **V8** |
| the low traverse into Chalk Ghost | **V8** |
| the blank wall behind the parking | **V10** |

**Nothing between V4 and V8.** The player clears the first two at V5 in their
first season and then has nothing to aim at for thirty years. A project is
supposed to be the thing you might do in your strong years if you commit —
and there isn't one.

## The fork, which is content and therefore Evan's

1. **A project at V6–V7.** Pitched at the top of a peak career, so a
   committed player gets one plum and the V8s stay as the ones that got
   away. Smallest change, directly serves "the player wins more", and fills
   a hole the grade table makes obvious.
2. **Soften one V8 to V7.** Same effect, no new content, but it takes a line
   away from the ones that are meant to be beyond you.
3. **Raise the progression ceiling** so a peak career reaches V8. Changes
   every other measurement in the project and should not be done for this.

**Recommended: 1.** Not done here — the guidebook is content, and content
has been Evan's call every time.

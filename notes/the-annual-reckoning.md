# The annual reckoning

*2026-09-01*

`Sim/DirtbagTax.{h,cpp}` new, SAVE v48. `TAX-1` ported from the 2D source,
and the second item off the tier-one list in `notes/the-2d-audit-three.md`.

The port spent a fortnight building things that pay in lumps -- a circuit
with a season podium, a national team with a stipend, a comp board, a World
Cup and the Games -- and taxed none of it.

## What it is

A running total of the year's prize money, and one day a year the tax man
takes a fifth. That is the whole system, and **it is deliberately not
something you interact with**. It is a date you either saw coming or did
not: the money arrived in spring, you spent it, and the bill is in autumn.

A bill you cannot cover behaves like every other bill here -- cash floors at
nothing and the shortfall goes to `owed`, which the field's own note already
describes: *"a bill you cannot pay does not evaporate, it waits."*

## Wages are not in it, and that is the design

A dirtbag's shifts are cash, off the books, at a diner and a gear shop and a
rescue callout. The source taxes prize money and the stipend and nothing
else, and **that narrowness is the point**: the one legitimate part of the
career is the one part the government has heard of. Banked at the three
places the money lands -- a comp result, a season podium, the federation's
stipend -- rather than at the reckoning, because a total does not know where
it came from and wages must never reach it.

## The number that had to be re-derived, for the third time

The source puts tax day at offset **9 of an 18-day year** -- halfway through
it, explicitly away from the birthday so the two do not share a screen. This
port's year is 365 days and its birthday is day 1 (`AgeOn`). Porting the `9`
across would have put the reckoning **nine days after the birthday**, in the
first week of January.

That is the same mistake this project has now caught three times: `GYM-6`'s
season pulls, `GYM-8`'s youth gains, and now this. A number tuned against an
eighteen-day year is not a number, it is a fraction of a year wearing one.
Day 182.

## Two things the implementation had to get right

**A year with no prize money still settles.** Marking the year done is the
point of the call, not the bill. Without it the year is re-checked every
time the night tick runs and the first evening podium after tax day is
billed on the spot.

**And the same day twice is not a second bill.** A save reloaded on the
morning of the reckoning must not be billed again, which is why the year is
*stamped* rather than the total being trusted to stay at zero.

**The morning has to be able to say what happened**, and that took a change
of shape. The reckoning runs inside `SleepToNextDay` and the engine's Sleep
plumbs no return value through the Blueprint library -- so the bill is kept
on the ledger and rebuilt by `TaxNews` on the morning it happened, exactly
the way `WorldCupSeasonNews` already works. Without that the tax man takes a
fifth of the year in silence, and a system whose entire design is *a date
you saw coming* would have no moment at all.

## What measurement found, and it is worth stating plainly

**It barely bites, and the reason is a decision this port already made.**

The probe now banks prize money the way the engine does -- it did not, and a
probe that pays prize money the tax man never hears about is how a balance
number comes to describe a game nobody plays. Run out over thirty years on
the `comper` policy: **723 comps, one win, 84 podiums, a ranking peak of 721
-- and $1,256 of tax across the whole career.** About $42 a year, against
$162,349 of wages.

That is not a bug in the tax model. It is `CompDials`' own decision, written
down and measured months ago: *"comp prize money is not how a climber eats,
and the 2D game's own note says the circuit had quietly become economically
pointless when it tried to be."* A hundred and twenty dollars for winning
means twenty-four dollars for the government.

**Where it does bite is the top, which is the right shape.** A career on the
national team draws the stipend at every season close, and this port runs
about five circuit seasons a year -- so **roughly $4,500 of stipend a year,
and a $900 bill against it**, which is a sixth of a year's wages arriving on
one morning. That is exactly what a tax on prize money should feel like:
nothing at all until you are good, and then a real number you have to have
kept.

Nothing was tuned to make the journeyman's bill bigger. Raising comp prize
money to give the tax something to eat would be re-tuning a measured system
to justify a new one, which is the wrong way round.

## What is Evan's

Nothing here needs the editor -- the reckoning and its warning are said at
the van on the morning they happen, through the sleep spot that already
exists. **The one open question is whether the gym should be taxed.** The
source's `ownGym` is not, and this port followed it; but this port's gym is a
business with a balance sheet and a foreclosure, and a case could be made
either way. It is a balance pivot against the source, so it is logged here
rather than taken.

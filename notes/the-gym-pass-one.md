# The gym, pass one

*2026-08-24*

`Sim/DirtbagGym.{h,cpp}`, SAVE v39. `GYM-1` ported from the 2D source —
the P&L engine and the five levers — with the rest of the family named and
deferred.

## Scoping it from source changed what it is

The audit files gym ownership under **Coaching & mentoring**, and I said so
out loud before reading the source: *"the taxonomy says the late-career arc
where you stop being the climber and become the one who makes climbers."*

Wrong. It has **its own family, `GYM-1`–`GYM-12`**, and it is a business:
pricing, staffing, equipment, marketing, members, a balance, and a bank that
takes it back. The youth team is one **wing** you can build onto it later.

Building from the table row would have produced the wrong system. That is
the third time this project has been bitten by auditing a summary, and the
first time the source arrived in time to stop it.

## What pass one is

Five levers and a nightly tick:

| lever | shape |
|---|---|
| **price** — budget / standard / premium | free to change; $8/30 members, $14/15, $22/8 |
| **set mix** — beginner / all-comers / hardcore | free; leans the same axis the other way |
| **equipment** — 0 / 1 / 2 | a capital ladder, cumulative, only goes up |
| **staff** — front desk, setter | a hire price and a daily wage |
| **campaign** — flyers / social | discretionary, time-limited, one at a time |

`members` drift **15% of the gap to target** each night. The original's
comment: *"not an instant jump — pricing takes several days to fully show
its effect, same 'meter, not switch' feel as training load elsewhere."* This
port already had that idiom three times over (`Thread::warmth`,
`Local::known`, `PartnerBond::rapport`), so it went in without argument.

Fourteen consecutive days in the red and the bank takes it, with a standing
hit — and the counter **resets the moment you are not in the red**, which is
what makes a fortnight a grace period rather than a countdown.

**Deferred, each its own numbered family**: the staff as named people with
traits and raise asks (`GYM-3`), rival gyms pulling your members (`GYM-4`),
the season (`GYM-6`), incidents (`GYM-6`), wings (`GYM-12`), the league you
run, hosting a comp, the youth team. Pass one models staff as two booleans,
which is **what `GYM-1` itself shipped** — the source says a save from
before `GYM-3` "derives a plain 1.0-quality staffer on the base wage".

## Measured

Thirty years, six seeds, gym bought the day it is affordable and the levers
left where they start:

| | buys on day | balance after | cash at the end |
|---|---|---|---|
| greedy | **never** | — | $178 |
| hoarder | 283 | $327,808 | $60,138 |
| salary | 528 | $320,368 | $579,917 |

**A climbing-first career cannot buy one**, which is correct and is the
same sentence the medical system says.

And the balance runs to $327,808 while the player's cash is unchanged —
which looks wrong and is not. **Checked in the source: every cost is paid
from the player's wallet and nothing is ever paid from the gym's balance.**
Wings, leagues, the comp bid, the youth foundation, staff, kit, ads,
incident choices — all of it. The only interaction with the balance is
`coverGymDebt`, paying *in* to clear a shortfall.

So a gym does not pay you. **It absorbs money by giving you a permanent
stream of things worth spending on**, and the balance is how you know
whether it is working.

That matters for the ceiling argument this was reversed on. Pass one
absorbs about **$41,000** — $25,000 for the building and roughly $16,000 of
levers — against a hoarder's $60,378. **Most of the absorption lives in the
parts not yet ported**: wings, a league to run, a $2,200 comp bid, the youth
team. The argument holds; it needs pass two to fully land.

> *2026-08-25.* Pass two landed it. The five wings add **$22,600** of
> capital, taking a fully-built gym to **$61,100** against that $60,378 —
> see `notes/the-gym-pass-two.md`. The $41,000 above was two figures short:
> the equipment ladder is $8,000 cumulative and the two hires $5,500, so
> pass one's own absorbable total was **$38,500**, not $41,000.

## One thing measurement corrected on the way

The foreclosure test first hired both staff, on the theory that two wages
would sink a premium gym. It does the opposite: **a hire raises the target
it is paid out of**, so at premium rates the front desk returns $110 a day
for a $25 wage. Staffing does not sink a gym in these numbers — an empty
floor at full overhead does. The test now prices premium-hardcore
*unstaffed*, which genuinely does go under and stay under.

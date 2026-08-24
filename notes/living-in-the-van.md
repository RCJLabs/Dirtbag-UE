# Living in the van

*2026-08-24*

`Sim/DirtbagLiving.{h,cpp}`, SAVE v40. `LIFE-21` and `LIFE-22` ported from
the 2D source — grime, water, propane — and the one rule that gives grime
teeth.

## The rule is narrow and the source says so

A comment on the function itself: *"the only mechanical bite: what a room
full of people is willing to give you today."* Grime does not make you climb
worse. It does not cost money. It multiplies **social gains** and nothing
else — three quarters when you are ripe (60), half when you are feral (80),
and one the rest of the time, because *lived-in is what everybody in this
valley is*.

## It landed on a seam that did not exist a week ago

There are exactly three social gains in this port, and **all three were
built in the last two days**:

| gain | built |
|---|---|
| `WhoIsAround` — who turns up | this morning |
| `PartnerBond::rapport` — what a day together is worth | Tuesday |
| `Local::known` — whether the counter says anything | yesterday |

Grime multiplies all three and touches nothing else. **A system arriving to
find the thing it needs already built is the argument for porting
sim-first** — a month ago this would have had nothing to bite on and would
have shipped as a number that went up.

One detail worth keeping: in `WhoIsAround` it discounts **what you bring**
— your rapport and your Social axis — and leaves each person's own
`showsUp` alone. Trish being here on a Tuesday is not about you.

## And it wanted the stove

Phase 11 gave you a `Thread::Cooking` that makes meals cheaper. `EatMeal`
now checks `CanCook` first: **the discount is only real when the bottle
is**. A cook with an empty bottle pays the diner's price like everybody
else. Two systems built a day apart that wanted each other exactly there.

## What was cut on the way in, and why

The original's `LIFE-22` has a **house battery** — lights, the fan, the
phone. It is not here.

Its only real consumer in the source is `POWER_PHOTO`, the camera, which
belongs to the media economy: a recorded cut past 1.0. So a battery ported
now would drain six points a night, hit zero in a fortnight, have nothing
to recharge it and nothing to spend it on. **A meter that only goes down and
does nothing is worse than no meter, because it looks like a system.**

`check-dials.py` is what stopped it: `powerCap` was read by nothing and said
so. The checker was built for exactly this and had never caught a live one
before.

## The numbers, from source

Grime: +6 a night out here, +5 for a day of chalk and rock dust, −3 when the
sky washes you. Ripe at 60, feral at 80. A rag and a jug takes 32 off and
**floors at 15** — you cannot get properly clean out of a jug, which is the
whole reason the truck stop is worth $8. The lake takes 50 and an afternoon.

Water: 40-litre cap, 3 a night, 3 to cook, 8 to wash, $2 a refill. Propane:
8 to cook, 6 for a cold night, $18 a bottle.

**One thing this port does not have is rain.** The 2D game rinses you when
it rains; this game models humidity. Rather than invent a rain flag to make
one dial fire, the rinse reads `ConditionsDials::humidityBite` — the line
the game already uses for *humid enough to matter*.

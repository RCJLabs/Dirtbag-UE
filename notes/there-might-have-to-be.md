# There might have to be

*2026-08-25*

`Sim/DirtbagYouth.{h,cpp}` new, SAVE v44. `GYM-8` ported from the 2D source.

Piper's second stage has ended the same way since the floor shipped:

> *Her mother asked if the youth team has a waitlist. There is no youth
> team. There might have to be.*

The source wrote that line and left it hanging on purpose. This is what is
behind it, and the whole system is gated on having actually lived that arc
out — founding it first would be answering nobody.

## Why it is worth more here than it is there

The source calls this *"the only system in the game where the thing you
build outlives you in somebody else's name"*: a kid you coached from eleven
turns up in the national field as a rookie, and from then on they are one of
the names you are chasing.

This port's national field is a fixed table rather than a roster, so that
promotion cannot be ported as written. It does not need to be, because
`Sim/DirtbagRival.h` has carried this comment since long before there was a
gym:

```cpp
// Somebody steps up. Sometimes it is a kid from the gym.
Rival Succeed(...)
```

So a graduate goes on a queue, and the next time the rival generation turns
over, the successor has their name. **The port foretold this system before
it had one.** Everything else about them is rolled the way a stranger's is —
you coached them to sixteen, not into a style — but the name on the board is
one you chose off a list of fourteen when they were eleven.

## The numbers are re-derived, not copied

The source tunes its per-session gains against a year that is **eighteen
days long**. This port's year is three hundred and sixty-five.

Copied across, a squad would reach the top of the ladder in about six months
against a five-year age gate, so **the birthday would bind in every single
case** — and the source's own comment records that exact bug as the thing it
tuned away from:

> *The first cut ran at 0.34/session, which meant every kid maxed out long
> before their sixteenth birthday — so the age gate bound in every case,
> every squad graduated at exactly 16, and coachCraft and the hired coach
> made no difference to anything.*

What is ported is the intent, stated there in one sentence: *"the CLIMBING
is the gate when you don't know what you're doing and the BIRTHDAY is the
gate when you do."* At one session per three days that is 852 sessions over
seven years and 487 over four, so:

```
gainBase        +  compBoost/5              = 10 / 852
gainBase + 12c  +  compBoost/5              = 10 / 487
```

Measured back out of the finished system: **7.0 years green, 4.1 years
practised.** `TestTheClimbingIsTheGateUntilYouKnowWhatYouAreDoing` asserts
both ends, so a future edit cannot quietly collapse it back into an age gate.

## Three things measurement found

### The hired coach did not coach

`check-dials.py` reported `YouthDials::gainHired` read by nothing, which was
true, and tracing why turned up the same hole in the source.

`runYouthSession` returns early when `coach === 'hired'`, and **nothing else
in the game ever runs a session.** `youthGain`'s hired branch is reached from
exactly one place: the panel that shows the player their projected gain. So
in the original, handing the squad over means the kids stop progressing
entirely, for $30 a day, behind a readout displaying a rate that can never
happen.

The stated design is unambiguous — *"you can pay somebody and let it run
without you, and the second one is worse for the kids and better for your
week"* — so here it runs. A hired coach's session goes in the night tick, at
`gainHired`, on the same three-day schedule; it teaches you nothing and earns
you no standing, because their weekend in the van is not your weekend.

Measured over thirty years and six seeds, which is what the trade was always
supposed to be:

| | your hours | somebody you pay |
|---|---|---|
| sessions run | 3,529 | 3,540 |
| kids put through | **24** | **16** |
| oldest graduate | 17 | **20** |
| coaching craft, end | 12 | 0 |

Two-thirds as many kids, and they leave at twenty instead of seventeen —
held four extra years by the climbing rather than the birthday. Worse for
the kids. Better for your week.

## Two more things measurement found

### The squad ran out of children

Fourteen written names, a squad of four, and four kids who train together
graduate in a clump every four or five years. A thirty-year career therefore
puts **twenty-four kids** through the gym — and the written pool is spent at
about the halfway point, after which the squad dwindled to nothing and the
system quietly ended.

The source cannot hit this, because its careers are a few hundred days long.

So past the fourteen the squad keeps filling: a name off a longer list and
one of the written tags, both derived from the recruit's number so a save
always meets the same kid. When that list comes round too, an initial —
which is what a squad with two Sorrels in it does anyway. You stop knowing
them by heart; they do not stop turning up.

### The queue was handing the seat to a forty-one-year-old

The first cut popped the front of the queue. A successor starts at
`RivalDials::rivalStartAge - 2` — twenty-four — and a kid who left at sixteen
is that age about eight years later. Popping the front handed the seat to
somebody who graduated twenty-five years ago; popping the back would have
handed it to a seventeen-year-old.

So it takes whoever is **closest to the age the seat wants**, and forgets
anybody past it by more than four years — they had their chance and went and
had a life instead.

That fix is what makes the original sentence literally true. Before it,
every successor was a graduate, all the time. Measured after it, over six
seeds and thirty years:

| | |
|---|---|
| generations that turned over | 2–3 |
| ...of which were kids you coached | **1–2** |

*Sometimes* it is a kid from the gym.

## What it measures out at

Thirty years, six seeds, `gym=life rival=1`:

| | |
|---|---|
| founded on day | ~333 (about a month after buying the building) |
| sessions run | 3,529 |
| kids put through | 24 |
| oldest graduate | 17 |
| coaching craft, end | 12 of 12 |

Craft maxes out inside the first six months of coaching, which means the
craft gate is really *your first squad* and every squad after it gets the
four-year version of you. That is the source's shape and it reads correctly:
you are worse at this than you will ever be again, exactly once.

## The trade, which is the building's own

Deliberately borrowed from the gym one floor down: you can run it with your
own hours, or you can pay somebody. A hired coach brings them on at about
nine years a kid instead of seven, costs the gym **$30 a day on its own
books** — so `GymDay` now takes an `otherWages` argument, because the gym
does not know it has a youth team — and gives you your evenings back.

`GYM-12`'s wing table also gets its `youth` column back. It was cut in pass
two on the rule that a dial nothing reads is a dial that lies; the kids' area
and the training annex read it now.

## What that leaves

`GYM-9` (bidding to host a circuit round, and then the choice between
running it and climbing it) and `GYM-10` (the league you run, whose format
decides who wins, because every regular leans one way). Both are scoped and
neither is started. The wing table's `bid` column is still cut, and comes
back with `GYM-9`.

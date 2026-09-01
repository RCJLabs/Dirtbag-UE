# Three small ones, and the number they caught

*2026-09-01*

SAVE v50. `DEPTH-8`, `DEPTH-19` and `CLB-32` ported -- the small end of tier
one. None of them is a system; each is a hole in one.

## `DEPTH-8` -- you learn a grade by getting on it

The port has had sandbagged lines since Phase 2. *Second Breakfast* has said
V2 and climbed V3 for three years, `ReadRoute`'s own comment says *"the
guidebook's opinion, not the rock's -- you can't see a sandbag"*, and **there
has never been any way to stop being fooled.** A line that spat you off nine
times still read "comfortable" on the tenth.

Now it is on the ledger: `ProjectMemory::knowsTheGrade`, set by **touching**
the line rather than sending it. That is the source's rule and it is right --
you do not have to do a route to feel how hard it is, which is the whole of
what a sandbag is.

Two small things it got right on the way past. The read is `ReadRouteKnowing`
and `ReadRoute` is that with `false`, so the golden vectors go through the
old sentence unchanged. And `WhatItReallyIs` is **empty when the book is
right** -- most guidebook grades are, and *"it is exactly what it says"* is
not a sentence anybody says out loud.

## `DEPTH-19` -- and half of it was already built

Climbing harder costs more to sustain. The source has two clauses: the daily
nut rises with your grade, and harder lines eat rubber faster.

**The second one has been in this port since Phase 3**, as
`GearDials::wearPerGradeOverFive`, under a different name and with nothing
recording that it was half of anything. The test now pins the pair together
so they cannot drift apart.

The first one is the half with teeth, and see below.

## `CLB-32` -- the film study

Eight energy and no time, once, for a comp that is on the calendar. It is
**not a reveal** -- the rival's style and vibe have always been visible; this
turns tendencies you already knew about into a plan.

Two rules make it worth having rather than free. It lives on the **board**,
not the climber, so it lasts the whole comp rather than one go and is gone
when the board is. And `TakeTheScoutingIn` moves it off the climber as it
lands, so the next comp is not free.

It is gated on its own reason and not on another function's silence, which
is the mistake `HostACompNight` made and this project has now written down
twice.

## The number this pass actually caught

`DEPTH-19` needed measuring, so I read the probe's bills line. It said:

```c
const double bills = DAYS / dd.billsEveryDays * dd.billsAmount;
```

**That is the dial, not the charge.** Three multipliers sit between them --
the Desert Local's discount, whatever your habits cost you, and now the creep
-- and the probe has never seen any of them. Every *"bills $132,940"* in
these notes is a calculation dressed as a measurement, and the money-shape
claim built on it (*"98% of everything earned goes straight back out"*) was
resting on a figure that could not move.

Seventh instance of **the tool's own coverage being the bug**.

The fix is not just instrumentation: **nothing has ever told the player what
the week cost either.** `PlayerState::lastBill` records it and `BillLine`
says it on the morning it lands, the same way the tax bill and the World
Cup's season news are read across Sleep. The probe now tallies the real
charge.

And the first cut of that tally was wrong in an instructive way: I put it
inside a block that only runs on days you pulled on, which counted about a
twelfth of the bills. **One wrong number replaced by another** -- $11,041
against a true $203,662 -- and only caught because the number was absurd.
It sits right after the night tick now, outside every climbing conditional.

## What the creep costs, measured

Thirty years on the `greedy` policy, with the dial at zero and at 0.08:

| | bills | broke days | ending cash |
|---|---|---|---|
| creep off | **$132,940** | 2 | $194 |
| creep on | **$203,662** | 7 | $157 |

**+53%, +$70,722 over a career, and it more than triples the days you are
broke.** That is a real squeeze and it arrives exactly where it should --
late, once you are climbing hard enough for the life to have got more
expensive around you.

**One thing to flag rather than tune.** The source justifies the size of this
dial with *"income scales faster, so it's a creep not a crush."* In the 2D
game income does scale with grade -- going pro, sponsor money, content. In
this port wages are wages: a shift at the diner pays the same at V12 as it
did at V4. So the source's safety argument does not straightforwardly hold
here, and whether +53% is a creep or a crush depends on an income curve this
port has not measured against it.

Left as it is, with the number written down. It is a balance call against the
source and therefore Evan's -- and the honest version of that call needs the
income measurement, not a smaller dial.

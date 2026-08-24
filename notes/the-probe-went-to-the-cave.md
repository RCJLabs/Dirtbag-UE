# The probe went to the Cave

*2026-08-24. Sport has existed since Phase 8. No career had ever climbed
one.*

The probe has only ever been to Roadside, so the belayer gate, what rapport
buys, the clipping economy and the cave's whole north-facing reason to exist
had been measured by the harness and never played. That is the same gap that
hid the World Cup's calendar (sixty Games in ten years) and the tooth's dead
end (10,688 days of abscess) — and both of those were *how often* questions
no assertion can ask.

Teaching it to go found four things. **Two were the probe lying about the
game; two were the game.**

---

## 1. The probe teleported to the crag

`Crag::approachHours` is set by all four crags. The engine charges it —
`ApproachHoursFor` feeds the travel spot — and **the probe has never paid a
minute of it.**

Every career number in every note in this repo was measured by a climber who
arrived at the rock instantly. Now paid, it is **2,637 to 3,874 hours over
thirty years**: a hundred and ten to a hundred and sixty days of a career,
spent walking, previously free. On the buttress it is 2.2 hours out of a
fourteen-hour day, every day.

**Two things I got wrong on the way to this**, both caught by checking
rather than by asserting: I first thought `approachHours` was read by
nothing at all, and then that it duplicated the zone travel model. Neither
was true. The engine reads it, it is not a duplicate, and the only thing
wrong was the instrument.

## 2. The probe's rapport call read the career, not the day

```cpp
SpendDayWith(p, t.daysClimbed > 0);   // the career total
```

From the first climbing day onward, **every** day counted as a day spent
together — including the nine in ten a career spends washed out, working or
resting. Rapport therefore pinned at 1.00 with everybody at the Lot inside a
fortnight and stayed there for thirty years, which is why the belayer's
patience ended a session **twice in four hundred days**: `BurnsTheyWillHold`
was returning the full-rapport number from about the first week.

The engine has always had this right (`SpendDayWith(P, bClimbedToday)`). The
probe has not, and everything it ever reported about the Lot was measured on
a player who never missed a day.

*(And my first metric for it was wrong too: `bestRapport` took a maximum
across the whole career, so it reported 1.00 for anybody who ever had a good
fortnight, thirty years after it stopped being true. **A high-water mark is
not a state.**)*

---

## 3. Rapport decayed to nothing, and every career ended a stranger

With the probe fixed, the real number came out: **0.00 with everybody, in
every career — including one that climbed 2,085 days**, which is nearly six
years of turning up.

The cause is that rapport was a hundred-day memory with hard clamps at both
ends, and **the clamps destroy the arithmetic.** A burst of climbing pins you
at 1.0; a long gap floors you at 0; the integral never matters and where a
career ends up is decided entirely by its last few months. Careers end old
and not climbing, so they all end at zero.

So `BurnsTheyWillHold` has only ever returned the stranger's three, and
*"somebody who knows you gives you the day"* — `DirtbagSport.h`'s own
description of the first thing in this game that rapport buys and nothing
else can — **has never once happened.**

**The fix is a memory, not a slope.** `everKnew` is a high-water mark that
never falls, and the drift now runs down to `rapportKeeps` (0.45) of it
rather than to nothing. You do not forget somebody you spent five years
with; you stop being current with them, which is a different thing.

Third time this project has arrived at the same shape from a different
angle, after the ranking's rolling window and the scars' fade: **a value
that ages needs a memory, not just a slope.**

Measured after: rapport ends at **0.45** in every career instead of 0.00,
and a belayer gives about seven burns instead of three. `belaycap` — days
that ended because somebody had had enough rather than because your skin
had — is **6 to 14** over a career, where it was 1 to 3.

SAVE v35 carries it, and its migration is **the only one in the file that
has to read the save to write it**: bonds are a counted list, so there is no
fixed set of keys to add. A v34 career knew people at least as well as it
knows them now, which is what the runtime would derive on its next day at
the Lot anyway.

---

## What this did not fix, and I stopped rather than guessing

**Rapport still does not distinguish one career from another.** It ends at
0.45 for a career that climbed 99 days and 0.45 for one that climbed 2,085,
because `rapportPerDay = 0.08` reaches the ceiling in thirteen days — so
both rapport *and its high-water mark* saturate early and carry no
information about a life.

The number that would fix it is `rapportPerDay`, and it cannot be moved
alone: rapport also feeds `ShareBeta`'s generosity, `PsycheFrom`,
`CrewDay`'s *"rapport that counts"* threshold, and the campfire's
`rapportPerHand`, which at any slower climbing rate would become the only
real source of rapport in the game. That is a design call about **where you
get to know people — at the fire or on the rock** — and it is four systems
wide. It belongs to Evan, not to the end of a long session.

**And the belayer gate has never once bitten.** `nobelayer = 0` across three
thirty-year careers: nobody has ever walked up to the Cave and found nobody
willing to tie in. `minRapportToBelay` is 0 and `LotRegulars` always returns
climbers, so `BestBelayer` always finds somebody.

That is the roadmap's own standing question — *"the Lot never varies, the
same three neighbours across ninety years"* — now with a number against it,
and it is the same hole that leaves Phase 7's `social` axis unwired. Turnout
is the missing mechanic and it is a design question, not a wiring job.

---

## What the Cave costs, measured

Same three seeds, boulders against bolts:

| seed | boulders | the Cave |
|---|---|---|
| a | 26 sends, 294 days | **7 sends, 128 days** |
| b | 18 sends, 2,085 days | **6 sends, 240 days** |
| c | 23 sends, 1,452 days | **6 sends, 99 days** |

Not a clean comparison and it is not meant to be — a rope route is longer,
the approach is longer, and somebody has to come with you. What it is, is
the first time any of that has been paid for.

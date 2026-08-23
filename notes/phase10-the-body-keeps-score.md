# Phase 10 — The Body Keeps Score

`Sim/DirtbagMedical.{h,cpp}`, SAVE v30, and the care counter at the gear
shop. Pass 1: diagnosis, treatment, the staged comeback, what a joint
carries, insurance, and the undertreated path.

## One sentence

**An injury hides its grade until you pay to look at it.**

Everything else follows. A climber with a sore finger does not know whether
it is three weeks or three months — they know it hurts. Every decision they
make is made without the number, and the number is exactly what would make
the decision easy. So the sim keeps the severity and hands the player a
description, and buying the number is itself the first decision.

That is what turns Phase 3's injury — *eleven days, physio buys six back,
nothing to decide* — into the first gate: **a decision with a wrong answer,
not a wait.** The wrong answers are all available and all tempting: skip
the diagnosis and guess, take the shot because it works *now*, come back at
the first stage that does not hurt.

## The three clocks

- **The comeback** is staged — resting, mobility, a graded return — and it
  advances because you say so, not because a timer ran out. Waiting a stage
  out is never a gamble; pushing on early is, and the odds get worse the
  earlier you go. Three days early you usually get away with it; the
  morning after a bad one you mostly do not.
- **The joint** carries what you did to it. Cortisone ends the acute stage
  outright and marks the joint permanently; scars from healed injuries
  flare for the rest of a career and fade slowly and never to nothing.
  Surgery is the only thing that takes damage back off a joint, and nobody
  operates on a guess.
- **The policy** you either bought before you needed it or did not.

## The four gates

**1. A decision with a wrong answer.** Diagnosis is three tiers: guess
blind (free, and the fog stays), a physio's hands ($60, close and *not
always right* — the error is the whole mechanic), or the machine ($340,
exact, and the only thing that unlocks an operation). Then rest, physio,
the shot, or the knife.

**2. A career can be shortened by choices made while injured.** Measured,
thirty years, `stakeout` climbing policy, two medical policies identical in
everything else. Sends over the career:

| seed | sensible | impatient | impatient's joint risk |
|---|---|---|---|
| crag-1 | 23 | 23 | 2.60 |
| north-2 | 29 | 25 | 2.60 |
| west-9 | 24 | **5** | 2.60 |
| crag-4 | 25 | 22 | 2.60 |
| south-3 | 20 | 14 | 2.60 |

Never better, and at west-9 the career is destroyed: 44 injuries against 6.
The impatient climber is *hurt for fewer days* — they come back early and
take the shot — and ends up with a body 2.6× as likely to break and a fifth
of the sends. **The cost is deferred, not avoided**, which is the whole
point.

Note the impatient climber usually has a *higher* allround grade, because
they climb far more burns. Strong and broken is a real thing and the sim
now produces it.

**3. Insurance is a real bet.** A thirty-year career pays about $6,250 in
premiums. Measured claims ran from $816 on a lucky body to $8,704 on an
unlucky one — so it is right on the career that got hurt a lot and a slow
expensive mistake on the one that did not.

It was $26 a fortnight first: $20,332 over thirty years against a maximum
measured claim of $8,976. **Never right, in any seed.** That is not a bet,
it is a tax with a story.

**4. The undertreated path is reached by being poor.** A career earns about
$7,200 a year and ends thirty years of it with $159 in the tin, so
cash-on-hand is the binding constraint and not lifetime income. The
`careful` policy — scan everything, operate when the scan says so — spent
**$0 in thirty years uninsured**, because it could never afford the $340 at
the moment it was needed, and went untreated on every single injury: 41
injuries and 5 sends at west-9. The same policy insured got 6 injuries and
24 sends.

**Insurance is what puts real medicine within reach of a dirtbag.** That is
the truest thing this phase says and it fell out of the measurement rather
than being designed in.

## The bug that took three findings to kill

**Two owners of one flag.** `BodyDay` counts `daysLeft` down and clears the
injury — that was the whole injury model through Phase 3. The staged
comeback is a second clock over the same flag, and they disagreed.

The probe found it three times in one afternoon, each time louder:

- **316 cortisone shots, hurt 10,696 days of 10,950.** The comeback reached
  its last stage, the injury was still flagged active, and the medical tick
  started the whole thing again the next morning. Forever.
- **294 surgeries out of one injury**, once a career had money. Nothing
  said no on the second morning.
- And when the loop was fixed, `NextStage` still only healed on the
  *waiting* path — a climber who clicked through the last stage came out
  the other side still flagged hurt.

Fixed three ways: `Injury::staged` says who owns the clock, `FinishInjury`
is the one place an injury ends, and one treatment per injury.

**None of the three had a test until the probe found them**, and two of the
three reintroductions initially passed — the `staged` flag was untested,
and `rushedComebacks` is reset by the heal, so a test that reads it after
the fact reads zero either way. Both are pinned now, and the second is
pinned on the *scar*, which is where the difference is actually observable.

## The other bug worth recording

Scars faded geometrically. The nightly fade read the weight it had just
written, so three years was a thousand multiplications, and the floor —
being a fraction of the current value — collapsed with it. **A value that
ages is a function of the date, not a field you keep editing.** Same lesson
as the ranking one phase ago, in a different file, within a day.

## Written and never wired, again

`StagePenalty` was declared, tested and never called — so the graded return
was not graded, it was just a longer wait with a nicer name. The checker
caught it. It is the number that makes the last stage *climbing*, and it is
the reason the comeback is staged rather than timed.

## What is left of Phase 10

Pass 2: **sickness**, **dental** (the 2D game's staged clock that only
escalates), **prehab** (lowering risk before the fact), **meds**, and **the
shrink**. None of them is load-bearing for the four gates, which pass now.

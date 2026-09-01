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

---

# Pass 2 — the things that are wrong with you that are not the injury

`Sim/DirtbagAilments.{h,cpp}`, SAVE v31. Sickness, the tooth, prehab, the
shrink.

## Why these four

Pass 1 is about one event and its consequences, and **every injury in this
game is something you did.** That is deliberate — an injury must never be
weather — and it leaves three shapes unbuilt, all of which the 2D game has
and all of which are a different kind of wrong.

**Sickness is not your fault**, and that is its entire job. It is still not
a die: it arrives off how you are living, so a fed, rested climber in a warm
van essentially never gets ill and a hungry one sleeping cold on a wrecked
body does. That is not a dice roll about them, it is a description of them.
Measured over 4,000 nights each: **living well is most of the answer, and it
is not immunity.**

**The tooth only ever goes one way**, which makes it the exact inverse of an
injury, where ignoring it is sometimes fine. Nothing improves it with rest.
The only thing that ever has is money, and it costs more at every stage —
$70 for a filling, $780 for the root canal it becomes. It is the game's one
pure test of whether you will spend on something that is not climbing.

**Prehab is boring, it works, and nobody does it.** Twenty minutes of a
morning, no money, and a streak rather than a total. It cuts a third off the
odds of a tweak and **never removes them** — a climber who has done their
twenty minutes every morning for a year still pops a pulley.

**The shrink** is the only thing in the game that buys psyche back, and it
does something a rest day cannot: it moves where psyche drifts back to
overnight, for a month.

## The measurement that changed a system

The tooth as first built was **a silent career-ender wearing a tooth's
coat.** It escalated on a clock and then never resolved, so a career that
would not pay sat at an abscess — 1.2 grades and 18 energy a day, forever.

> **10,688 days of 10,950 with an abscess. Four sends over thirty years,
> against twenty-four.**

Nothing in the harness could see it: every claim about the tooth was true
(it escalates, it never improves, it costs money) and the system was still
wrong, because *how long you sit at the end of it* was unbounded.

Two fixes, and the second is the real one:

- The abscess costs less — 0.5 grades and 9 energy. It has to hurt enough
  that you pay, and not enough that not paying is the end.
- **An abscess resolves itself, the worst possible way: the tooth comes
  out.** That is what bounds never paying to the better part of a year of
  misery and a tooth, rather than the rest of a career. `Teeth::lost` counts
  them, because a career remembers.

After: 5,460 tooth-days over thirty years if you never pay — half a career
with something wrong in your mouth, cycling — and 0–2 if you deal with each
one while it is still a filling. Sends back in the 21–29 range everywhere.

## What the probe says

Thirty years, `stakeout`, three medical policies:

| | ill | sick days | tooth days | injuries | sends |
|---|---|---|---|---|---|
| control | 86 | 633 | 5,460 | 7 | 23 |
| sensible | 84 | 622 | 5,460 | 2 | 24 |
| sensible-up | 89 | **356** | **0** | 1 | 22 |

Eleven dollars of meds nearly **halves the days lost to being ill**
(633 → 356), which is the cheapest good decision in the game and is
deliberately still a decision. Prehab cut injuries at three seeds of four
(7→1, 17→11, 10→3); the fourth reshuffles because a different injury
timeline changes everything downstream of it.

Two to three colds a year across every seed, which is a person.

## Written and never wired, caught before it shipped

`PsycheFloor` was declared, tested and never called — so the shrink's whole
point, *it moves where you drift back to*, did nothing at all, and it would
have shipped as a $110 button that buys the same thing a good night's sleep
does. The checker caught it. Third time this class has appeared in two
phases.

Two dials the checker caught the same way: `sickEnergyCeiling` and
`toothAbscessEnergy` were both read by nothing. **You do not wake up at a
hundred when you are ill**, and an abscess takes the morning — both belong
in `WakeUp`, which until now ignored the player entirely.

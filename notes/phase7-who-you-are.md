# Phase 7 — who your climber is

**2026-08-23.** Evan: *"yes, then phase 7"*.

**Every climber in this game was identical at birth.** `NewClimber` rolled
five skills within ±6 of fifty and a body type, and that was the whole of a
person. The 2D game asks four questions before you touch rock — where you
came from, what kind of climber you are, what is wrong with you, and what
you are like — and then rolls two things behind your back that you find out
by playing.

`Sim/DirtbagCharacter.{h,cpp}` is those six things.

## Two rules, and only one of them is zero-sum

**Archetypes redistribute and never add.** The five offsets of every
archetype sum to zero, so choosing one is choosing a *shape* rather than a
score and no archetype is the strong one. This rule arrives already paid
for: the 2D game learned it on its competition field, measured the version
without it, and fixed it by shifting the offsets. A set of offsets that is
not zero-sum is a difficulty setting wearing a costume, and a test enforces
it.

**Origins deliberately do not.** Where you came from is a life, not a build.
A Desert Local really is better at rock and worse at pulling hard than
somebody who quit an office job, and a Trust-Fund Kid really does start with
$400 and a valley that can smell it. They are not balanced on one axis
because they are not *on* one axis: **each holds one permanent perk in a lane
no other origin touches** — shift pay, indoor gains, training gains, daily
costs, shop prices, physio — and **none of them goes near send odds.** That
last part is what keeps "where you came from" out of the resolver, and a
test asks every origin for its odds penalty and requires zero.

Only a **flaw** is allowed near odds, and only one of them uses it.

One substitution, logged: the 2D game's Late Bloomer perk is *35% off health
insurance*, and there is no insurance in this build — it is Phase 10. A perk
in a lane that does not exist is a field nothing reads, which is the one bug
this project keeps finding, so it moved to the nearest lane that is real and
has a door: **physio**, which since yesterday you can walk up to and buy.
Same intent, and it moves back when insurance lands.

## The bug this phase found in itself

The first version had no `built` flag. `Build` has to default to *something*,
and whatever it defaults to carries that origin's perk and that flaw's cost.
It defaulted to an All-Rounder who Sold It All with Gumby, which meant —
measured, not guessed:

> **every existing caller trained technique at half rate and got paid 12%
> more per shift.** The probe. The golden vectors. Every measurement in every
> note in this repo.

**All 75,027 checks passed.**

They passed because the training tests are *orderings* — "the grinder ends
stronger than the cruiser" — and halving both sides preserves an ordering.
That is this project's own recorded lesson (`magnitude vs ordering`) arriving
from a direction nobody was watching: not a test that was too weak for a new
feature, but a test that was exactly strong enough until a default changed
underneath it.

**An unbuilt character is nobody, and nobody gets no modifiers.** Every
effect function returns *exactly* neutral — not "close to", exactly, because
the point is that adding identity to this game must not move a single number
that was measured without it. `MakeCharacter` is the only thing that sets the
flag, and a test pins all eleven effects at neutral without it.

It also gives old saves a free answer: a v20 career migrates to **unbuilt**,
which is exact rather than generous. The climber you had keeps the numbers
they had, and the creation screen does not ambush somebody twenty years in.

## Measured

Twenty-four paired seeds, 365 days, same seed both sides so the ±6 of luck is
identical and the gap is the build.

**Archetypes change what you climb, not how good you are.**

| | W/T/L | means |
|---|---|---|
| Boulderer vs Rope Gun — **grade** | **24 / 0 / 0** | 5.99 vs 4.85 |
| Boulderer vs Rope Gun — allround | 17 / 0 / 7 | 5.48 vs 5.43 |

A Boulderer out-grades a Rope Gun **on every single seed**, by over a full
grade, and their average of five skills is **identical to two decimal
places.** That is exactly what a zero-sum table should produce, and it is the
strongest evidence the rule is doing its job.

**Sends are the wrong metric and it is worth saying why.** Unpaired across
twelve seeds a Boulderer scores anywhere from **0 to 25 sends**; the build
means (2.3 to 7.1) sit well inside that. Whether a season produces sends is
mostly whether the seed's crag has a line at your grade. Paired comparison on
grade answers the question; an unpaired sends count never could.

**Flaws bite where they say, and the training ceiling damps them.**

| | W/T/L | means |
|---|---|---|
| Tweaky Fingers → injuries | 16 / 6 / 2 worse | 3.71 vs 2.50 |
| Gumby → technique | 23 / 0 / 1 worse | 53.77 vs 56.60 |
| Hard Gainer → power | 21 / 0 / 3 worse | 52.19 vs 53.34 |

Tweaky Fingers is **+48% injuries over a year** — a real, felt cost. But
Gumby trains technique at *half rate* and lands only **5% lower after a
year**, and Hard Gainer only 2% on power. **Diminishing returns eat most of a
flaw**: `headroom` shrinks gains as a skill climbs, so halving the input
mostly buys you a slower approach to the same ceiling. Recorded rather than
tuned — it may well be correct that a flaw slows you rather than caps you,
but it means the *rate* flaws are much weaker than the *risk* one, and the
blurbs currently promise them as equals.

**Origins are visible on day one and invisible in the bank.**

| origin | power | fingers | tech | end | head | cash |
|---|---|---|---|---|---|---|
| Sold It All | 48.2 | 51.5 | 53.6 | 53.9 | 49.3 | 294 |
| Gym Rat | 52.2 | 55.5 | 53.6 | 49.9 | 43.3 | 127 |
| Desert Local | 45.2 | 51.5 | **57.6** | 51.9 | 52.3 | 163 |
| Ex-Gymnast | **54.2** | 53.5 | 51.6 | 51.9 | **42.3** | 182 |
| Late Bloomer | 44.2 | 49.5 | 56.6 | 51.9 | 52.3 | 214 |
| Trust-Fund Kid | 48.2 | 51.5 | 55.6 | 51.9 | 47.3 | **394** |

Ten points of spread on power — about **a grade and a half** — before a day
is played.

**But end-of-year cash does not separate at all**: Trust-Fund against Desert
Local is **11 / 2 / 11** and $159 against $162. That is not a Phase 7 bug, it
is the economy this project already measured — *98% of everything earned goes
straight back out and the balance never clears $450* — so a starting float
and a shop discount both wash out inside a season.

**The perk does land, just not where a cash column looks.** Over twelve
seeds the Trust-Fund Kid climbs on **19% less worn rubber** (shoe wear 0.314
against 0.388), because 30% off means replacing it sooner. Dead rubber is the
thing that costs sends (`notes/shoes.md`: never replacing shoes takes a
season from 3.7 sends to 1.0), so the money shows up in the *condition of
your gear* rather than the balance. That is a better outcome than a cash
number and it was not designed — it fell out of the treadmill.

## The talents, and why you are not told

One gift and one anti-talent, on **different skills**, rolled at birth. A
climber whose fingers both come fast and come slow is a wash, which is not a
character, and the roll re-rolls rather than filtering — a filtered list would
change the odds of the remaining anti-talents depending on which gift landed,
for no reason anybody designed.

**They are live from the first move and hidden only in the knowing.** That is
the design stated as a test: nobody is told they have good tendons, they find
out over ten years of having had them. Forty sessions in a lane and it
becomes obvious — so **a gift in a skill you never train stays a secret
forever**, which is the point: you learn what you are by what you do.

The line the game says is a notice, not a reward: *"A season of fingers work,
and it is starting to be obvious. Bomber Tendons: cabled tendons — your
fingers shrug off load."* No numbers, no "unlocked". And the creation screen
deliberately says nothing about them at all.

## The door

Four questions on the same furniture as the handover, because **in this game
a new career is an arrival** — the first climber and the one who turns up
after you retire are answering the same question, so they go through one door
rather than two that will drift apart.

Keys **1–6** (three more than existed; six origins need six keys, and a
question offering an option with no key to press is the bug the wall's ethics
prompt had two days ago). **E** only works on the last screen, because "press
any key" on a question with a right answer is how people skip past the choice
they were meant to make. It saves immediately: four answers is enough of a
decision that losing it to an alt-F4 would be a real annoyance.

## What is not in this phase

**Two of the four personality axes have readers and two do not.**
`discipline` bends what a session teaches you (Purist beats Send-or-Bust on
technique 19/24) and `purism` bends what a shift pays. `social` (whether
people turn up) and `boldness` (what you commit to above the last piece)
want seams in the partner and sport models this phase does not touch — so
they are **stored, saved, set and checked, but not consumed.** Written down
here and asserted in the suite rather than shipped as two multipliers nothing
multiplies. Wiring them later is a one-line change each, not archaeology.

**Quirks and habits are not here.** They are earned from a tally of how you
actually climb rather than chosen, which needs instrumentation in every
session — a coherent second pass, not a corner of this one.

**Personality does not drift yet.** In the 2D game the axes move with how you
play. Same reasoning: it needs the same instrumentation as habits.

## Verified by reintroduction

Ten defects, each put back and counted:

| defect | caught |
|---|---|
| a non-neutral default | the whole neutral-pin block |
| archetype table not zero-sum | 1 |
| two origins sharing a lane | 2 |
| an origin reaching into send odds | 2 |
| gift and anti-talent on one skill | 2 |
| a talent that never rolls | 1 |
| an effect that waits until you know | 1 |
| a flattened archetype table (still zero-sum) | 3 |
| a dead training multiplier | 5 |
| the save dropping the character | 3 |

The last one matters more than it looks: without it, `bBuilt` would load as
false and **the creation screen would reopen on every load, forever.**

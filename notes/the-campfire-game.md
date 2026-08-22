# The campfire game

**2026-08-22.** Last of the four unported systems. `concepts/DIRTBAG.md` §4
cuts the 2D game's minigames to *"keep ONE campfire game"* and names poker as
the example. This is that one, and it is **a design, logged as one** — the
repo has the sentence and nothing else.

## The fire had no verb

`SitAtTheFire(Hours)` already gives rapport, psyche and a line of talk. It is
entirely passive: you spend hours, you receive, there is no decision and
nothing you can be bad at. Meanwhile **5,056 days of a thirty-year career
never come good**, and the evening of a washed-out day has nothing in it.

## Why money is the stake now, and would not have been last week

When the campfire game was cut, cash had no destination — losing some was a
rounding error and poker would have been flavour. **Dreams changed that.**
The float is what a dream costs *and* what keeps the van off the bodge, so a
bad night at the fire is felt twice. The minigame got its teeth from a system
built three days after it was cut.

## Why rapport is the skill

You are not reading cards, you are reading people. `DealHand` gives you your
own hand exactly, and each opponent's hand **blurred towards a coin-flip by
how little you know them**:

    read = truth * q + noise * (1 - q),  q = 0.15 .. 0.85 by rapport

At no rapport you are mostly looking at 0.5 — which does not lie to you, it
tells you nothing. At full rapport you are looking nearly straight at it, and
still not quite.

So the social system carries the minigame instead of a card simulator bolted
on the side, and there is a **mechanical** reason to sit at one fire for a
season rather than a sentimental one.

The player never sees a number. `ReadText` turns it into a sentence — *"Dev
is not even looking at his cards"*, *"Trish has gone quiet"* — because a
number would make this arithmetic and it is meant to be a person.

## It was free money, and measuring caught it

First build, 20,000 hands × 5 seeds, dollars per hand:

| | never folds | plays the read |
|---|---|---|
| rapport 0.00 | −$0 | **+$14** |
| rapport 1.00 | −$0 | **+$27** |

**A stranger who simply folded when behind made $14 a hand.** The cause: the
Lot always matched your raise, so folding cost the $5 ante and winning was
paid by three people who never got out of the way. The fire would have been
an infinite cash machine bolted to an economy where money buys van uptime and
dreams.

`theyStayAbove = 0.45` — the Lot folds what it would fold. Raise on a monster
into a weak table and you win the antes and nothing more, which is what
betting big with the nuts actually gets you.

After:

| | never folds | plays the read |
|---|---|---|
| rapport 0.00 | **−$5** | +$3 |
| rapport 0.50 | −$5 | +$6 |
| rapport 1.00 | **−$5** | **+$10** |

A passive player bleeds. A thinking stranger scrapes. A thinking friend
earns. **Inattention has a price and knowing people is the skill.**

## The test only caught half of it

Reintroduction found something worth writing down. Deleting the fold rule
left **every ordering check passing** — a stranger still beat a passive
player, a friend still beat a stranger — while the table quietly paid $14 a
hand to anybody with eyes.

**The bug this game actually had is a magnitude bug, and orderings cannot
see magnitude bugs.** The test now pins numbers: passive < −$2, stranger <
$6, friend < $15. With the rule removed it fails on two of them.

That generalises past this file. A balance test that only checks *A beats B*
will pass on a game where everybody prints money.

## Known exposure, not measured away

At full rapport with the whole Lot, a good night is roughly a day's wage.
Whether a player who grinds cards every washed-out evening for thirty years
can earn a dream's worth is **not measured**, and the honest answer is
probably yes. The bound is the evening — hands cost hours at the fire, which
compete with rest — and `maxStake` is the dial if it turns out to matter.
Worth a probe policy before it is worth a change.

## Still open

Whether poker is the right one. §4 names it as an example, not a decision,
and the alternative I keep coming back to is **calling grades** — somebody
describes a line, everybody guesses, the guidebook settles it — which would
use the sandbagging the game already models (`grade` vs `trueGrade`) and
teach the player to read rock. It is a better fit for the subject and a worse
fit for the stake, because nothing is at risk. Both could exist; §4 says keep
one.

# Liar's dice and blackjack

**2026-08-22.** Evan: *"No I like the games we already have. Liars dice.
Poker. Blackjack. All stay."*

That reverses §4's *"keep ONE campfire game"*, logged as
`concepts/PIVOT-campfire-games.md`. Poker was already built; these are the
other two.

## They are not one game three times

| | what it asks | what rapport does |
|---|---|---|
| **poker** | read the person | it **is** the skill |
| **liar's dice** | is he lying, and dare you say so | sharpens the tell; nerve does the rest |
| **blackjack** | one decision against the odds | **nothing** |

Blackjack earns its place by having no social component at all. It is the
game for a climber who has just arrived somewhere and knows nobody — a real
state in this game, and one that nothing else pays off. Every other thing at
this fire is gated on people.

## Liar's dice

Five dice each, ones wild. Somebody bids the table holds at least N of a
face; the bid reaches you and you have one decision: **call it, or pass it
on.**

Measured, dollars per round, 40,000 rounds × 4 seeds:

| rapport | pass | call everything | play the tell |
|---|---|---|---|
| 0.00 | −$5.00 | −$22.56 | −$7.69 |
| 0.50 | −$5.00 | −$22.56 | −$3.44 |
| 1.00 | −$5.00 | −$22.56 | **+$6.88** |

Passing costs the ante — sitting at the table costs whether or not you do
anything at it. Calling everything is punished hardest of anything in any of
the three games. **Reading a stranger is worse than not playing**, which is
right: you cannot tell whether somebody is lying if you met them on Tuesday.

## Two magnitude bugs, both caught by measuring

**`bidAmbition` started at 1.35.** The bid sat well above what the dice
actually held, so nearly every bid was a lie — **calling blindly won $19.21
a round**, and playing the tell was *worse* than reflex-calling at low
rapport, because a careful player passed up free money. A liar's dice bid has
to be true more often than not, or the bluff is not a bluff. Now 0.85.

**The tell was a perfect lie detector.** I reused poker's blur —
`truth*q + noise*(1-q)` — on a *binary* question, and the two cases stop
overlapping at any `q >= 0.5`: a lie lands above 0.5 and a true bid below
it, every time. Rapport 0.5 and rapport 1.0 scored **identically to the
cent**, which is the tell that something is saturated rather than tuned. It
also made a nonsense of my own comment about a friend of ten years still
getting you now and then.

Symmetric noise around a signal instead, so the ranges always overlap: at
full rapport a liar reads 0.45–1.05 and an honest bid −0.05–0.55, and the
sliver in the middle is the friend still getting you.

## Blackjack

You against the deck, the Lot watching. Hit or stand; the deck plays itself
out to seventeen. No shoe — a campfire deck is shuffled every hand and nobody
is counting, and modelling a shoe would make card counting the skill when the
point of this one is that it has **no skill you can build, only a decision
you can get right**.

| stand on | $/hand |
|---|---|
| 12 | −$5.71 |
| **15** | **−$5.07** |
| 17 | −$6.19 |
| 19 | −$12.59 |
| 21 (never stop) | −$25.00 |

**Played well it costs you the ante and nothing more.** That is the intent:
the game with no edge to build, not the game that punishes you for sitting
down.

It took two goes. At `blackjackPays = 1.2` with ties going to the player it
was **profitable at every strategy anybody would use**, which is not a game,
it is an ATM. Ties push now and the pay is 1.05 — flat 1.0 left good play at
−$5.91 against a $5 ante, so the cards themselves were costing you money on
top of the seat.

## What this run taught, again

Every one of the three balance bugs across these two games was a
**magnitude** bug that orderings would not have caught, exactly as poker's
was. All three are now pinned by number in the tests and all three fail on
reintroduction:

- `bidAmbition` back to 1.35 → **3 checks fail**
- the old blur → **1 check fails** (`halfTell < friendTell - 1.0`)
- ties to the player at 1.2 pay → **1 check fails**

One near-miss worth recording: reintroducing the blur first appeared *not*
to fail anything. It had left `base` unused, `-Werror` rejected the build,
and grepping only for `FAIL` saw an empty result and read it as a pass.
**A reintroduction that does not compile is not a passing test**, and the
check for that is to grep for `error:` alongside `FAIL`.

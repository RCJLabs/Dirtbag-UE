# The fire gets a table

*2026-08-22. Phase 5, item 1.*

Evan, having played the desk list: *"all seems to work for the most part.
definitely will need to improve the fire games."*

The three campfire games shipped four days ago riding on a toast that
expires in twelve seconds. That was deliberate and it was written down —
`notes/the-campfire-game.md`: *"the toast is the whole table UI until an
evening of play proves the fire earns a widget."* An evening of play has now
proved it.

## What a toast could not do

Four things, and they are not cosmetic:

1. **Hold your hand on screen while you think about it.** Poker's whole
   decision is your hand against three reads. Twelve seconds is enough to
   read them and not enough to weigh them, so the honest way to play was to
   press C fast and stop thinking — which is the one thing this game is
   not for.
2. **Keep the reads beside the decision they inform.** Rapport is the skill
   these games are made of. A read that has scrolled away is a read you did
   not get, and the whole social system was being spent on information that
   left the screen before it could be used.
3. **Show a blackjack hand that takes four keypresses to finish.** Each hit
   drew a new toast over the last one. The one game at this fire with a
   multi-step decision had the worst possible surface for it.
4. **Say what the evening cost.** Nothing tracked it. You could lose two
   hundred dollars one $20 shrug at a time and never see a number that said
   so — and that number is the one that decides whether there is a next
   hand.

## What it is

Canvas-drawn in `DirtbagHUD.cpp`, on the same pattern as the climb wall's
session bar: the spot fills `FDirtbagFireReadout`, the HUD draws it, and a
real widget later binds to the fields the canvas already uses. **No
Blueprint work**, container-side, preflight-checkable, and consistent with
how everything else in this game is drawn.

It sits where the session bar sits, because the two are never both up — the
wall and the evening are the same slot in the player's attention.

The panel grows with what is on it rather than reserving room for the
biggest possible night: three reads at poker, one tell at dice, none at
blackjack, and no result line until there has been a result.

**Poker's hand and blackjack's total share a bar.** They are the same shape
of fact — how much have you got — and blackjack's is drawn against 21, so
the bar says how close to the edge you are, which is the entire question
blackjack asks.

**The verbs are named for tonight's game.** "stay in / throw them in" is not
"call it / let it go round" is not "another card / stick". Two keys, always
on screen, never worth memorising.

## Three bugs found on the way in

**Tonight's game was a rule living in the presentation layer**, recomputed
as `Day % 3` in four separate places — the prompt, the deal, the commit and
the back-down — with the prompt keeping its own separate list of names.
Four copies of one rule is three chances for two of them to disagree about
which game they are settling, and no test in this project could ever have
noticed. It is `dirtbag::WhatsOutTonight` now, with `GameName` beside it,
and it has a test.

**Walking away mid-hand was a free look.** Every game here takes the ante at
settlement rather than at the deal, so you could deal, read the table, and
step out of the trigger owing nothing. Standing up now folds — the same body
as F, because they do the same thing to the hand; the difference is only who
decided. The result gets a toast in that one case, which is exactly what a
toast is for: news, after the fact, with nothing left to decide.

**Sitting mid-hand would have deleted it.** Pressing E passes hours, hours
cross midnight, and a hand keyed on yesterday's day would simply stop
existing — unsettled, unpaid, gone with nothing said. It says *"Finish the
hand first"* now.

## The stake, and the lever with one end

The engine carried `CardStake = 20.0`: a number typed beside a dial rather
than one taken from it. The sim's ceiling is 40, so "the stake" was
permanently half of it and **there was no way to bet small on a bad hand**,
which removes the only decision poker has that is not folding.

So the fire's 1/2/3 set the stake — the same three keys that name a dream at
the gear shop, and the same question in both places: *how much of the float
is this worth.* The notches come from `dirtbag::StakeNotch`: the ante, half
the ceiling, the ceiling. Retune `maxStake` and the keys follow.

Poker and liar's dice let you size the bet with the hand in front of you —
that *is* the decision. **Blackjack locks at the deal**, because raising
once you have seen your cards is not a game.

And then measuring turned the feature down. With four rapport levels and
20,000 hands each:

```
              notch0 ($5)   notch1 ($20)   notch2 ($40)
stranger        +0.009        +0.893         +3.346
half-known      +0.027        +2.174         +5.229
familiar        +0.550        +3.545         +7.537
friend          +1.125        +4.998        +10.169
```

**The ceiling won at every rapport, by a factor of ten.** Press 3, always.
The Lot called a $40 shove exactly as often as a $5 nudge, so the stake
scaled the winnings linearly and never flipped sign. A control whose only
correct setting is "maximum" is not a decision; it is a lever with one end,
and I had just built one and nearly shipped it.

The dial's own comment already said what should happen — *"a big raise into
a weak table wins the antes and nothing else"* — and it did not, because
nothing read the size of the raise.

`shoveMakesThemFold = 0.30`. What the table needs before it will call now
scales with what you shoved: at the ante they need `theyStayAbove`, at the
ceiling they need that plus 0.30. Shove and they lay down, so you win the
middle and no more; nudge and they call, so a good hand gets paid. Downside
is untouched either way, because losing costs what you put in whatever the
table thought of it.

**0.30 is the smallest swept value that works.** At 0.15 and 0.20 the
ceiling still beat the middle for a friend (+5.38 against +3.77, +4.01
against +3.38); at 0.25 they were within noise of each other (+2.84 against
+2.99); at 0.30 the separation is clean (+1.83 against +2.60).

Measured after:

```
                 notch0 ($5)   notch1 ($20)   notch2 ($40)
passive              -1.30         -5.00          -5.00
stranger             -1.12         -0.79          -2.31
friend               +0.94         +2.60          +1.83
```

## What this changed about the game

Two things, and Evan should know about both.

**The fire pays less.** A friend playing the read used to make $10 a hand
and now makes $2.60 at the best stake. That is a four-fold cut to a system
whose measured numbers are already in the changelog. It moves in the
direction every previous measurement of this system worried about — the note
from four days ago flagged *"whether a player grinding cards every
washed-out evening for thirty years earns a dream's worth"* as an unmeasured
exposure — but it is a balance change, not a bug fix, and `theyStayAbove`
and `shoveMakesThemFold` are both single dials if he wants it back.

**A stranger now loses money at poker.** It used to pay +$3. This is the
change I am most confident about, because it makes poker agree with the game
beside it: liar's dice charges a stranger $7.69 for exactly the same
ignorance, on the stated grounds that *"you cannot tell if somebody is lying
when you met them Tuesday."* Poker was contradicting its own table.

## The check that would have caught it

The campfire's original lesson was that a balance test which only checks "A
beats B" passes on a game where everybody prints money. This is the same
lesson from a third angle: the *ordering* checks on the poker test all
passed at every stake, at every value of the new dial, including zero. What
fails is the property itself — `ceilFriend < friendly`, `ceilStranger <
stranger` — and setting `shoveMakesThemFold` back to 0.0 fires exactly those
two and nothing else.

One process note against myself, and it is the second time: the first
attempt at that reintroduction edited the comparison out of the loop
directly, which left `demand` unused, `-Werror` rejected the build, and a
grep for `FAIL` saw nothing and read as a pass. **A reintroduction that does
not compile is not a passing test** — it was in the changelog four days ago
and I still did it. Grep for `error:` alongside `FAIL`, every time.

## At the desk

`CardStake` is gone from the fire spot's details panel, replaced by
`StartingStakeNotch` (0, 1 or 2 — default 1). Nothing needs re-placing; the
default is the old behaviour.

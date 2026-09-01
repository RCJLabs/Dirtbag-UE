# Pivot: the campfire games all stay

**2026-08-22. Decided by Evan.**

## What the concept doc said

`DIRTBAG.md` §4, under *Cut or defer past 1.0*:

> comps/Olympics, expeditions, deep-water solo, big-wall multi-day,
> filmmaking/photography economy, **minigames (poker etc. — keep ONE
> campfire game)**, gym ownership, Solo mode, Notown/Halloween. The 2D game
> took years to accrete these; the 3D game earns them the same way.

I built one — poker — on that basis (`notes/the-campfire-game.md`).

## What Evan said

> *"No I like the games we already have. Liars dice. Poker. Blackjack. All
> stay."*

So the cut is reversed for this item only. **Three campfire games ship**:
liar's dice, poker, blackjack. The rest of §4's cut list is untouched.

## Why this is the right call and the doc was wrong

The cut was written on a scope argument — *"the 2D game took years to
accrete these"* — which is true of comps and expeditions and **not true of
these three**. They already exist, they are already balanced, and they are
cheap in a way the rest of that list is not: no new art, no new animation,
no new places, no new economy. Card and dice games are the one category on
the cut list that is pure sim, which is the half of this project that runs
ahead of the other.

There is also a design reason that only became visible after the fact. The
three are **not the same game three times** — they differ in what they ask
of the player:

| | what it asks | what rapport does |
|---|---|---|
| **poker** | read the person | it *is* the skill |
| **liar's dice** | is he lying, and dare you say so | it sharpens the tell, nerve does the rest |
| **blackjack** | one decision against the odds | **nothing** — no people in it at all |

Blackjack earns its place precisely by having no social component: it is the
game for a climber who has just arrived somewhere and knows nobody, which is
a real state in this game and one nothing else pays off.

Cutting to one would have thrown away a spread that took no design work to
get, because it was already there in the 2D game.

## Consequence

`Sim/DirtbagCampfire.*` was written for one game and is restructured to hold
three. Poker's rules and its measured balance
(`notes/the-campfire-game.md`) are unchanged.

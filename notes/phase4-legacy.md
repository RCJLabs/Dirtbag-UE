# Retiring, and what a career leaves behind

Aging landed last, so a career could decline. It could not end — and a
climber who cannot stop is not a career, it is a treadmill.

## The design call

> **The world remembers. The body does not.**

Nothing physical carries over. The next climber is twenty-four with a fresh
body, nothing in the fingers, one crash pad and $300, because inheriting
somebody else's tendons is nonsense and would make the second life a
save-scum of the first.

What carries is everything *outside* the body:

- **The lines you put up** are in the guidebook under the names you gave
  them, with your FA credit and what they actually went at.
- **The Lot knows whose van that is.** 35% of your standing transfers, and
  it is *signed* — inherit a career that annoyed the stewards and you arrive
  already annoying them. Being somebody's kid brother cuts both ways.
- **The gate opens.** A closure is not inherited; a crag that stays shut
  forever is a dead crag rather than a consequence.

That is the version of legacy this game can actually mean. A first ascent
only becomes permanent when somebody else has to climb past your name.

## Nobody is ever thrown out

Retirement is offered, never forced. Deciding when to stop is the last real
choice a climbing career contains, and taking it away would be the one
unforgivable thing to do to one. The game only starts being *honest* with
you, on two triggers:

- **A body that keeps breaking** — three injuries back to back, at any age.
  It says it before the numbers do.
- **Two grades off your best, sustained, past 46.**

**Age alone is never the reason.** Plenty of people climb their hardest at
forty, and telling one of them to pack it in because of a birthday would be
both wrong and insulting. A 49-year-old still climbing at their peak is
told nothing at all.

## What needed a save version

`ProjectMemory` learned which ladder its line is on. A guidebook entry for a
rope route reads *5.13c* and one for a boulder reads *V7*; a career card
that cannot tell them apart confidently prints the wrong ladder, which is
the sort of thing a climber notices instantly and never forgives.

**SAVE_VERSION 12** carries that plus the legacies themselves. A v11 save
predates the rope crag entirely, so every line it ever touched was a boulder
— not a guess, the only thing that could have been true — and nobody came
before it. Tested against a real v11 fixture generated from the v11 writer
rather than hand-typed.

## The bug this nearly shipped with

The engine's save path took `(Seed, Player)` and nothing else, so every
legacy added to the game instance would have been written to disk exactly
never. Nothing would have looked wrong until somebody opened the guidebook
and found three generations missing.

This is the same shape as the three per-day ticks that were written and left
uncalled (`KitDay`, the body roll, `FactionDay`). Fixed the same way — not
by remembering to pass them, but by making `SaveGameToFile` /
`LoadGameFromFile` the complete path and routing the game instance through
it, with the narrower Blueprint pair documented as first-life-only.

## One honest note on a test

Injecting "remove the closure reset in `Inherit`" did not fail any test —
because `Inherit` builds a fresh `PlayerState`, whose `closedDays` is
already zero. The line is a no-op that documents a decision rather than
performing one, and its comment now says so, so the day somebody copies the
whole `Standing` across, the decision is there to be found rather than
silently reversed.

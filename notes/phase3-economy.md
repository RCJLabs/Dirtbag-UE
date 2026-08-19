# The money loop — baseline and first coupling

Phase 3's gate: *"the money loop pressures the climbing loop the way the 2D
game's does."* This is what it did before anything was built, and what it
does now.

## The baseline: no pressure at all

A year, played headless (`Sim/tools/build-season.sh`):

| | |
|---|---|
| earned | $4,860 from 81 shifts |
| bills | $4,420 |
| dog | $729 |
| food | **$0 — the player never ate once** |
| days worked | **22%** |
| cash low | $93 |

Two things wrong with that. **Nothing was ever at risk** — one shift in
four or five covered everything, and no decision in the whole year was
forced by money. And **bills have nothing to do with climbing**: a flat
weekly drip is solved by working more, so the money loop and the climbing
loop never actually meet. Working harder is always available and always
sufficient.

The `food $0` line is its own finding: hunger runs at 3/hour and a day only
reaches about 17:00, so it tops out near 30 against a meal threshold of 45.
Meals were effectively optional. Fixed in the probe by eating at 28; worth
watching whether hunger should climb faster, since a day that ends at 5pm
is doing less work than the dial assumed.

## Shoes: the first cost that scales with climbing

Rubber wears **by the move**, and faster the harder you pull, so the expense
tracks exactly the thing the player wants more of. Dead rubber costs grades
before it costs money — 1.1 at zero, squared, landing hardest on edging
holds — so a worn pair pushes you onto slopers and jugs long before it stops
you. The decision is the dirtbag one: resole at $55 while the uppers hold,
$165 when they do not, two resoles to a pair.

The same year, with shoes and with the eating threshold fixed:

| | |
|---|---|
| earned | $5,580 from 93 shifts |
| bills | $4,420 |
| food | $432 (54 meals) |
| dog | $729 |
| **shoes** | **$330** (3 resoles, 1 new pair) |
| days worked | 25% |
| **cash low** | **$11** |

## Being honest about what did what

Cash bottomed at $11 rather than $93, which looks like the gate moving. Most
of that is the **eating fix**, not the shoes: food went $0 → $432 and shoes
only account for $330 of about $5,900 spent. Shoes are 6% of the year.

What shoes actually bought is the *shape* rather than the size: a cost that
rises when you climb more, which is the coupling the gate is asking for and
which bills can never provide. It is the right mechanic and it is not yet
enough of it.

Also note **0 days on dead rubber** — the probe's policy resoles the moment
wear passes 0.55, so it never once climbed on bad shoes. The grade penalty
is built and tested but has not been exercised by a real career. A player
short of cash will meet it; this policy never is.

## What is still missing

The economy has no **lump**. Every cost is a small steady drip that working
one more shift absorbs. The 2D game's pressure comes from bills landing when
you are broke and from things breaking — and **van wear and breakdowns on
crag drives** are in Phase 3's scope precisely because that is the mechanic
that takes a season away from you in one afternoon. That is the next bite,
and it is the one likely to move the gate.

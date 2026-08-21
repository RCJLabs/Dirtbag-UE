# Staking a claim

**2026-08-21.** Evan: *"add the v6-v7 project."*

Added. Then measured it, and the measurement said something better than the
thing it was asked to check.

## The line

`the sit start to Shade Line`, book guess **V6**, Technical, at Roadside.

Shade Line is V4 and one of the best things at the crag, so its sit start has
been brushed by everybody and done by nobody: too hard for the locals who
love the crag, too soft to interest the visitor who came for Send Train.

Technical rather than Power on purpose. Across a measured career power
collapses to 16.8 while technique ends *higher* than it started
(`phase4-career.md`), so this is a line you can still come back for at fifty.

It is **appended** to `kProjects` rather than slotted into grade order,
because the drift draws from a stateful stream and inserting it mid-list
would re-roll every project after it — changing the difficulty of lines
players are part-way through. Pinned by a test.

## It is a well-known line, and the book says so

The wide drift (`-1/0/+1/+2`) put it at V8 in 9.5% of worlds, which is the
one outcome that defeats the point of adding it. The fix was already in the
file: the cave's projects use a tighter drift because *"a bolted project's
guess is better informed than a boulder's — somebody has already hung on it
putting the bolts in."*

Same logic, now a field. `ProjectEntry::wellKnown` gives `-1/0/+1` at
`20/55/25`, and a line everybody has pulled on cannot be out by two.

| | V5 | V6 | V7 | V8 |
|---|---|---|---|---|
| wide drift | 25.0% | 32.0% | 33.5% | **9.5%** |
| well-known | 21.5% | **53.0%** | 25.5% | — |

Send rate, clean and wired, at crag-3 (where it lands V7):

| line | grade | 45 | 55 | **65** | 75 |
|---|---|---|---|---|---|
| the sit start to Shade Line | V7 | 0.0% | 3.3% | **77.3%** | 98.5% |
| Send Train | V8 | 0.0% | 0.0% | 9.7% | 85.1% |
| the arete left of Diesel | V9 | 0.0% | 0.0% | 0.5% | 58.7% |

Pitched exactly where it was meant to be: the top of a peak career.

## [corrected] The hole was not universal

`phase4-the-lot.md` said Roadside's projects run *"V3, V4, V8, V8, V10 —
nothing between V4 and V8."* **That is crag-1, not Roadside.** Measured
across 500 worlds, **92.2% already had an open project at V5–V7**, because
the guess-5 cattle grid and the guess-7 arete drift into the band. crag-1 is
one of the unlucky 7.8%, and I generalised one seed into a structural claim.

The sit start takes it to **100%**, which is a real improvement and a much
smaller one than the note implied.

## And the player still did not win more

Ninety years, four generations, six seeds, `projector` policy:

| | player | the Lot |
|---|---|---|
| before the new line | 6 | 11 |
| after | **6** | **15** |

Adding a line adds it to the Lot's pile. The player's share did not move at
all. **Content was not the constraint, and neither was the Lot's strength.**

The Lot rolls for a first ascent every day from day one. The player cannot
touch a V7 until year ten or fifteen of a life. `SpokenFor` protects a line
you are visibly working — so the Lot has a decade's head start on every line
in the book, and takes it.

## What actually works, and it was already in the game

`SpokenFor` counts a **brushed** line as claimed:

> *"Anything you have pulled on, anything you have already put up, and
> anything you have brushed — taking a wire brush to a line is the most
> public way there is of saying you are on it."*

So a player has always been able to walk up to a project ten years above
their grade, spend half an hour on it with a brush, and have the Lot leave it
alone. Nothing in the game ever said so, and no probe policy had ever tried
it, because `PickLine` skips anything that reads `NotThisYear`.

A `stakeout` policy — `projector`, plus half an hour brushing each open
project you cannot climb — over the same ninety years:

| seed | projector | stakeout |
|---|---|---|
| crag-1 | 2 / 1 | 4 / 2 |
| crag-2 | 1 / 3 | 4 / 0 |
| crag-3 | 0 / 4 | 2 / 1 |
| crag-4 | 1 / 3 | 1 / 3 |
| crag-5 | 1 / 2 | 3 / 1 |
| crag-6 | 1 / 2 | 2 / 0 |
| **total** | **6 / 15** | **16 / 7** |

**From losing 2.5:1 to winning better than 2:1, with no rule changed.** The
brush time is real time out of a real window — the claim costs a burn, and
still pays.

Third time this exact shape has appeared: the crash pad, the projector
policy, and now this. The game already supported it; nobody had ever tried.

## So the answer to "make the lot less strong"

It was never the Lot. It was that the game keeps its best strategy a secret.

`ClaimText` says it once, on the press that crosses the threshold:

> *Word gets round. Nobody at the Lot will touch it now.*

Only at the crossing, so it is news rather than a label — and it needs no
project test, because an established line starts at cleanliness 1.0 and is
therefore already above the threshold. The same default that made the bare
`> 0.2` in `SpokenFor` fragile works for us here.

## Still open

The `stakeout` result is a *probe* policy, not a player. It proves the
strategy is available and worth 2.7× the first ascents. Whether a human
finds it now that one toast mentions it is a playtest question, and the
honest place to answer it is at the desk.

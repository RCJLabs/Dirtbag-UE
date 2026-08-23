# Phase 13 — Trad

*2026-08-23. Built out of order, at Evan's request, while Phase 6's
remaining half waits for a PC with an Editor on it.*

The cut ladder's last systemic rung, open since Phase 3 and deferred four
times on the same correct reasoning: a deeper game whose presentation is the
constraint is a worse game, not a bigger one.

## What it is

Trad is not a second climbing model. It is the **same runout model** —
Phase 8's, unchanged — reading protection that arrived by a *decision*
instead of by a bolter. That is gate 3, and everything else falls out of it.

The whole of the difference is three sentences, and a watcher can name all
three from the ground:

- **The gear is a decision.** A bolt is where the bolter put it, it is free,
  and it is bomber. A nut is where the rock lets you *and* where you choose
  to spend the pump, and it is only as good as the placement you made under
  it. Choosing not to place is a real option and sometimes the right one.
- **It costs more than a clip.** `clipPumpCost` is 3.0 from a jug;
  `placePumpCost` is 9.0, and from a blank wall with no feet it is 23.4 —
  more than the move you are standing on costs to climb.
- **You can run out of it.** A rack is finite. Sew up the bottom half and
  the headwall is a solo; save it all and you are a long way out for the
  first half. No policy is right on every pitch.

## What the numbers say

The eighteen-move crack pitch, a skill-70 climber (allround grade 7.8),
cams, 600–800 attempts a cell:

| grade | sport | trad |
|---|---|---|
| V5 / 5.11a | 0.960 | 0.890 |
| V6 / 5.11c | 0.737 | 0.478 |
| V7 / 5.12a | 0.167 | 0.028 |

**About a grade**, which is the number every trad climber will quote you
about their own two grades. And it is not one number, it is two reasons
that show up on different rock:

| | sport | trad | pieces | worst exposure |
|---|---|---|---|---|
| crack, V6 | 0.735 | 0.453 | 3.7 | 0.79 grades |
| face, V6 | 0.936 | 0.489 | 1.4 | 3.13 grades |

On a crack you pay in **pump** — there is gear everywhere and you stop for
it. On a face you pay in **head** — there is almost nothing, the bot places
1.4 pieces in seventeen moves, and the exposure is four times the crack's.
Same climber, same grade, same ladder, two completely different days. That
is why the buttress has both, and it is why one of its testpieces is called
*Ropeless in a Sense* and the other is called *Bombproof*.

And the rack is worth its money: doubles 0.453, cams 0.416, nuts 0.338 at
V6 on the crack.

## The four things measurement found

**1. Soloing was the strongest strategy in the game, and it was free.**

`AttemptInput` arrives fully padded by default (`padding = 1.0`), and below
the first piece the exposure model is the crash-pad model. So a leader who
placed nothing had *zero* exposure at every move of an eighteen-move pitch
and paid no pump for gear either. Measured: **soloing sent 68.6%, leading
sent 0%.** Every ordering test in the file passed.

Fixed with a solo term that is trad-only and says why: on a bolted route the
region below the first bolt is two moves by construction and the pad answer
is right there; on a trad lead it is however far you have chosen to climb
without stopping. It scales by moves off the deck rather than by fraction of
the route, because the ground does not care how long the pitch is.

**2. `soloGradePenalty` had to be the largest number in the resolver.**

3.2 grades, above `pumpGradePenalty` (3.0) and `injuryGradePenalty` (2.6),
and it should be: it is the only one of them that can kill you. Everything
else in this game prices *how hard the move is*; this prices what happens if
you get it wrong.

**3. The most expensive purchase in the game made you worse at climbing.**

The fear model saturated at the *sport* number: `RunoutAt` clamps to 1 and
was multiplied by `runoutGradePenalty` (1.1), so a piece you were certain
would rip could never be more frightening than a bolt runout. A worse rack
therefore made worse placements, the bot declined most of them, running it
out was free, and the pump was saved. **Nuts sent 0.651; a double rack of
cams sent 0.454.**

Fixed by `FallPenalty(trust, solo, runout, curve)` — one move on a slider
between the two penalties, so what a fall costs is priced by *what is going
to catch it*. A bolt's trust is 1, so it lands exactly on
`runoutGradePenalty` and no golden vector moves.

The curve had to be convex (exponent 2). Held linear, a full rack of cams —
which places at about 0.75 in a good crack — priced every runout as though
half of the fall would end on the ground, and a 5.12 climber could not lead
5.11. **A piece you are 70% sure of is not 70% of the way from a bolt to the
deck; it is a piece that will almost certainly hold.**

**4. Every placement on a crack route was priced as if made off a crimp.**

A crack move's `restQuality` is zero unless the hold happens to be a jug, so
the stance term punished the whole discipline where it should have been
free. The bot placed 2.2 pieces in eighteen moves, none better than *it
might hold*, and the send rate for leading was zero.

`PlacingEase` is `max(restQuality, TakesGear)` now — **a hand-sized cam into
a hand-sized crack goes in off a jam.** The rock being obvious buys you the
same thing a ledge does.

## The bot is a decision, not a threshold

The batch resolver drives the live one, exactly the way it takes every
shake-out — so the measured game and the played game cannot drift. Its
policy started as two thresholds ("place above this quality", "place a worse
one to get off the deck") and both were the wrong shape. A quality bar
cannot tell a good piece from a *useful* one: with a bare 0.24 bar for the
first placement, the bot got a nut in at move 1 that it believed in so
little the pitch came out **more frightening than soloing it** (worst
exposure 1.06 against 1.05). That is not a leader making a decision, it is a
leader with a habit.

It asks one question now: **is the stretch of climbing this piece is being
bought to cover less frightening with it than without, by more than the pump
costs?** Both sides are the runout model's own arithmetic, in grade units,
through the same two dials the resolver uses. Spacing falls out of it —
right above a bomber cam a second one buys nothing — and so does the
first-piece rule, and so does declining rubbish rock.

Two things it got wrong on the way, both worth writing down:

- **Averaged over the stretch, not read off its far end.** Read at the end
  alone, a piece that leaves you fully runout by the top of the stretch
  looks worth exactly nothing, however much of the stretch it made safe.
- **Rationing raises the bar; it does not stretch the lookahead.**
  Stretching it ran past the distance fear saturates at, every comparison
  came out "as bad as it gets either way", and a leader with two pieces on a
  fourteen-move pitch carried them to move ten and placed both next to each
  other. **Being short does not make you see further, it makes you fussier.**

## The gate that proves it is a decision

Three leaders on the same pitch, same seeds: one who never places, one who
places at every move that offers anything, and the bot.

    sensible  0.866      sew it up  0.000      solo  0.591

Both extremes lose. Soloing stays *tempting* — 0.591 is not a stupid
option — which is the shape a decision is supposed to have.

## The buttress

Sixteen lines and three unclimbed ones, an hour up the hill, east-facing.
Shaped like neither of the other crags and for reasons about the discipline
rather than the rock:

- **The spread starts low and stays low.** Nobody bolts 5.7 in a cave; a
  buttress has been climbed since before anybody owned a drill, and its
  classics are moderate. Half the list is inside a first season. **Trad is
  the one discipline whose entry-level lines are the famous ones.**
- **Almost everything is a crack**, because `RouteType::Crack` carries
  `HoldType::Crack` through `SignatureHold` and `TakesGear` reads the hold.
  A crack line is a line you can protect. Derived, not authored.
- **The hard ones are hard because of the gear.** *Ropeless in a Sense* is a
  face route on the list on purpose.

## What it cost elsewhere

- `Discipline` gained `Trad`, appended — the save stores it by value and the
  engine mirror `static_assert`s against it.
- `Protection` and `Rack` went into `DirtbagCore.h` rather than
  `DirtbagTrad.h`: the runout model reads protection and the resolver
  carries a rack, and neither of those files can include the trad one
  without a cycle.
- `AttemptResult` carries the gear. The staging needs it — a whipper onto a
  bomber cam and a whipper onto a nut the leader did not believe in are the
  same fall and not the same shot — and so does the day loop's head
  training, which would otherwise price a well-protected pitch and a solo
  identically.
- `PlaceGear` is a verb of the live attempt, beside `ShakeOut`, because it
  is a thing the player does mid-go with the clock running. That is gate 2,
  and it is why it is not a field on `AttemptInput`.
- SAVE v33 carries the rack. `SessionState` carries it too — the mirror
  checker's standing lesson applies: a skipped field is not merely absent
  from Blueprint, it is *erased from the sim* on the next round trip, and a
  leader whose rack was erased solos the pitch.
- The gear shop sells it: nuts $190, cams $640, doubles $1,150, one rung at
  a time, on **G**. The most expensive thing on any shelf in the game, and
  the only purchase that unlocks a whole crag rather than improving a day.

## A career of leading

*Added the same day. The gap above is closed: `Sim/tools/season.cpp` gained
a `trad` policy — saves for a rack, then climbs the buttress instead of
Roadside, buying up the shelf as the money arrives.*

Three thirty-year careers, each run twice on the same seed — once as the
ordinary boulderer, once as a leader:

| seed | sends | head | allround | work% |
|---|---|---|---|---|
| a | 25 → **9** | 61.6 → **77.6** | 5.40 → **6.02** | 13 → 14 |
| b | 17 → **8** | 66.7 → **100.0** | 5.56 → **6.63** | 14 → 14 |
| c | 25 → **8** | 62.1 → **69.9** | 5.84 → **5.32** | 14 → 14 |

**A trad career ticks about a third as much and ends a bolder climber.**
Head is up by 16, 33 and 8 points, and it is up because head is trained off
exposure and trad is where the exposure is — nothing had to be added for
that, it fell out of `ExposureAt` being the single source the day loop
already read. Allround is up at two seeds of three. Sends are down hard at
all three, which is the honest price and the same sentence a trad climber
would say about their own logbook.

Head reaching **exactly 100.0** at seed b is a cap-out, and worth flagging
rather than fixing here: head never declines, so a long enough trad career
trains the last decade for nothing.

### Two things the career measurement found

**The top two rungs of the shelf were decoration.** Measured with the probe
just buying what it could afford, all three careers bought a set of nuts on
day one for $190 and never once held enough cash to consider cams. The
reason is not the price, it is the shape of the money: **a dirtbag's balance
oscillates between about $125 and $290 for thirty years.** Peak cash never
approaches $640. Any purchase above about $300 requires deciding to work for
it, exactly as a dream does.

So the policy now works for the rack — the only reason anybody in this game
has turned a climbing day into a working day for a piece of equipment — and
with that, every career reaches doubles and spends $1,980 doing it. **Work
goes from 13% of days to 14%.** The rack costs a few weeks of a thirty-year
life, which is the right answer: it is a real decision and not a wall.

**The rack count could not matter.** Not one lead in 2,622 emptied the
harness, because 8/12/18 pieces is more rack than any 14-to-20-move line in
the valley needs — a pitch takes a piece every `botSpacing` moves, so five
or six. Even the *entry* rack rationed at 1.0, which means the rationing
arithmetic never once fired either. The whole "you can run out of it" half
of the discipline was provable in the harness and unreachable in a played
career: **the third time this project has shipped that shape**, after the
craft's sacking gate and the comp probe that had never entered a comp.

Sized against the pitches now — 5/8/12 — so a set of nuts genuinely does not
cover a line and doubles genuinely do. That is the second axis the shelf is
supposed to have, next to quality.

## Still open

- **A leader still never empties a full rack in a played career**, because
  the probe's grind falls low on most burns and doubles is twelve pieces
  against a pitch's five. Running out is now an entry-rack-and-long-pitch
  phenomenon rather than a general one, which is honest, but it means the
  played game rarely sees a mechanic the harness proves.
- **The comparison above is not clean.** The control climbs 5-to-8-move
  boulders and the leader climbs 14-to-20-move pitches, so some of the send
  drop is length rather than discipline. The clean comparison is the
  buttress against the cave, and the probe cannot go to the cave — it has
  only ever climbed Roadside.
- Head caps out at 100 in a long trad career.

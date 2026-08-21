> **Correction, 2026-08-21.** The claim below that north-facing makes this
> the only summer rock is **false as measured**, and its own next clause is
> why: the summer window lands at about 5:30am, before the sun is on any
> face, so North, South and West all give an identical 25 days at an
> identical 0.880 friction. Aspect only bites in winter, where south beats
> north 42 days to 28. The Cave is still the only rope crag and still costs
> a belayer — that is plenty — but it is not the summer crag. See
> `notes/phase4-crag3.md`.

# The Shaded Cave, and the belayer

Two halves of the same problem. After building the whole sport system there
was **not a single sport route in the game** — all thirty lines in the
guidebook are `Discipline::Boulder`, so the rope code was unreachable,
exactly where the town was that morning. And nothing anywhere required a
partner, so a pitch was just a longer boulder you could do alone at dawn.

## The crag

**The Shaded Cave** — eighteen authored pitches from 5.9 to 5.14a, plus
three bolted-and-never-climbed projects. Forty minutes up the hill, and
**north-facing**, which is the whole point of it.

Roadside is east-facing: sun on it all morning, good in the evening. The
season model puts high summer's window at dawn and nowhere else, and
`SunOnRock` returns zero for north aspects. So in July the cave is the only
rock in the valley worth walking to — the second crag is not more of the
same, it is the answer to a season the first crag cannot give you.

The grade spread is shaped like a sport cave rather than a boulder field:
far fewer easy lines (nobody bolts 5.7 in a cave), a deep middle where the
crag's reputation lives, and two or three testpieces most visitors only ever
hang on. Sandbags stay famous and specific — *Dogging It* is 5.13a in the
book and has never gone at less than 5.13c.

A bolted project's grade guess drifts tighter than a boulder's, because
somebody has already hung on it putting the bolts in. What is unknown about
a sport project is whether it goes at all, not roughly how hard it is.

## The belayer

**No partner, no pitch.** The first thing in the game that genuinely
requires the Lot to exist, and the thing that makes a rope route a different
*decision* rather than a longer boulder.

Who will belay you is not a courtesy:

| | burns they will hold |
|---|---|
| a non-climbing neighbour | 0 — Bo is not catching a whipper for you |
| somebody you have never spoken to | 3 |
| somebody you have spent a season with | 12 |

Anyone who climbs will hold your rope for a couple of laps, because that is
what people do at a crag. Standing under somebody all afternoon while they
work the same three moves is a favour, and favours are what rapport is.

This is the first place rapport buys something you **cannot get any other
way**. Until now it bought beta (which you could get by trying more) and a
little psyche (worth well under a grade). A redpoint burn is not available
at any price without somebody who knows you.

## One comment corrected

The cave draws from its own derived stream, and the first version of that
comment claimed it was what protects Roadside's rock from moving. It is not:
`BuildRoute` salts by route *name*, so a line's shape depends on its name
and nothing else — which is what makes routes stable without storing a move
list, and it means Roadside was never at risk from call order at all. The
derive buys something narrower and still worth having: two lines that ever
share a name stay different pieces of rock, and the project loop below it
draws from a stream that really is stateful. Corrected in place rather than
left as a plausible-sounding wrong reason.

## Still desk work

Belay, clipping and lowering are presentation, and rope staging is deferred
to MVP rung 2 in `concepts/DIRTBAG.md`. The cave also has no actor in the
level yet — it is a second `Venue` on a travel spot, the same shape as the
Crag/Gym split that already works.

# What you climb makes you

*2026-09-01*

`Sim/DirtbagStyle.{h,cpp}` and `Sim/DirtbagMonotony.{h,cpp}` new, SAVE v49.
`DEPTH-6`, `DEPTH-13`, `CHAR-6`, `CHAR-6b`, `CHAR-7` and `PSY-2` ported --
four tier-one items, because they turned out to be one idea.

**The port has had five trained skills since Phase 0 and a route type on
every line since Phase 2, and nothing has ever connected the two.** A career
of nothing but crimping and a career of nothing but dynos built the same
climber. The only thing a route type did was pick which skills the resolver
read.

## The idea, in one sentence

Repetition dulls, novelty pays -- at three scales.

**A style** (`DEPTH-6`). Every attempt hones one a little, a send teaches it
more. Over-index and it is a strength; neglect one and it rots.

**A skill** (`CHAR-7`). Feed a lane the same stimulus and its gains taper;
feed it a novel one and the monotony sheds, and breaking a real plateau pays
a burst on top.

**A place** (`PSY-2`). Climb the same walls day after day and everything the
*climbing* gives you dulls.

## Three things fall out of the style tally, and the third is the good one

**Odds**, with an asymmetry that is the whole design: a specialty is worth a
little, an anti-style costs nearly twice as much. Without that, specialising
is free.

**Gains, backwards from the odds, deliberately.** Your anti-style trains
fastest and your specialty plateaus. **This is the only thing stopping
specialisation being a one-way ratchet** -- the crimper who finally gets on a
dyno improves at it quickly, which is both true and the reason specialising
is a choice rather than a trap.

**A name.** Fifteen sends of a style and the scene starts calling something
after you; forty of a *different* one much later and there is a second. It is
the only thing in this game **the player names about their own climbing**,
and nothing is named without them -- the sim only ever says one is ready.

## The re-derivation, for the fourth time

The source states its style numbers as **odds** -- +0.07 for a specialty,
-0.12 for an anti-style. This port's `oddsPenalty` seam is in **grade
units**, because that is the currency every other modifier on an attempt
already speaks. Feed 0.07 into a grade field and a lifetime of specialising
is worth nothing at all.

So it was measured. On a limit route, at the part of the curve where a lean
actually decides something: a shift of 0.10 grades moves send odds 19.4% ->
23.9%, 0.25 -> 31.8%, 0.50 -> 45.7%. Nearer the top it flattens (a whole
grade is worth 24 points there against 51 here), so the honest conversion
across a career is about **0.35 odds to the grade**. The dials are the
source's numbers through it.

Fourth time: `GYM-6`'s season pulls, `GYM-8`'s youth gains, `TAX-1`'s tax
day, and now this. **The pattern is always the same** -- a number tuned
against something this port measures differently, which reads as a number and
is really a ratio.

## The bug the probe caught, and it was mine

The first cut fed monotony **per burn**. The source steps it per *session*,
and with eight burns to a session a lane saturates in under two days and
every session after the first sits at half gains, forever.

**The probe is what found it, and only because the tally was added.** The
career reported *"plateau: nothing stuck"* -- true at the moment it was
asked, because a rest day sheds monotony, and completely wrong about the
thirty years before it. An end-of-career reading of a thing that decays says
nothing. The probe now samples the peak and counts plateaued days, which is
the honest measurement and the one that would have caught it on day one.

Fixed: stepped once on the session's first burn, read once per burn.

**And a second, subtler one in the same place.** The stimulus was the route
*type* -- so an outdoor crimper's stimulus is `"crimp|rock"` every session
for thirty years, all five lanes saturate together, and a dial tuned against
a stimulus that changes is pointed at one that never does. It is **the
line** now. A plateau is what grinding one route gets you, which is a true
sentence about climbing and exactly what a projector does; a day at the crag
picking different lines is genuinely different work and pays like it.

## What measurement found

**Every career becomes a crimper, and the guidebook is not why.** Three
completely different policies over thirty years came out 78%, 86% and 89%
crimp -- and Roadside's lines are 21 crimp, 17 power, 16 technical, 15
endurance, 11 crack, 4 dyno. The skew is **emergent from climbing whatever
is at your limit**: the crag's mid-grades happen to be crimpy, so a career
that picks by grade becomes a crimper without ever deciding to. Every one of
them ends *"Known for crimp. Everybody has watched you on crack."*

That is `DEPTH-6` doing exactly what it is for, and it is the first thing in
this port that reads a career back as an identity rather than a total.

**Monotony is a light mechanic here, and that is honest.** The `greedy`
policy peaks at 0.70 and spends **5 days plateaued of 709 climbed**; the
`projector` peaks at 0.45 and never plateaus at all. It exists, it bites
occasionally, it never dominates -- and it will bite hardest on exactly the
player it should, the one grinding one line for a season.

**No regression.** A thirty-year `projector` ended at V3.2 with all of this
on and V3.3 with it off. Worth saying plainly: **that career goes backwards
either way**, from V5.0 to V3.3, and that is pre-existing and not this
pass's. It is worth a look -- the probe climbs 427 days of 10,950 and the
weather eats 5,056 -- but it is a separate finding and changing it here
would have hidden this one.

## What is Evan's

**The naming prompt.** `AMoveWantsAName` says a style is ready and
`SignatureKind` says what to call it -- *"your crimp sequence"*, *"your
dyno"* -- and `NameTheMove` takes the name. The text box is the same one
`notes/phase2-naming-widget.md` already specifies for first ascents, which is
the whole reason that note exists.

**And the venue string.** `PSY-2` counts sameness by `DayState::venue`;
`GoToTheGym` sets it and the engine should set it from whichever rock you are
standing at. Empty reads as "the crag", which is right for a sim-only caller
and wrong for a game with four of them -- so until the engine fills it in,
every outdoor day counts as the same place.

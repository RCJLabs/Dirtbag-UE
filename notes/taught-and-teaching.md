# Taught, and teaching

*2026-09-01*

`Sim/DirtbagMentor.{h,cpp}` new, SAVE v51. `WRLD-10`, `TUT-6`, `ROSTER-1`,
`COACH-5` and `JOB-10` ported. **Tier one is closed.**

The port has a rival, partners, a crew, locals, a youth squad and a gym full
of regulars. **Nobody in it had ever taught you anything, and nobody had ever
paid you to teach them.** Two halves of one thing, which is why they share a
file and a vocabulary: what somebody needs and what you can teach are the
same list, and the source reuses one set of five words for both on purpose.

## The half where you are taught

An old crusher notices you once you are climbing real grades and teaches you
five things in order -- footwork, reading the line, falling, resting, and
then everything she has left. Her last lesson is the design in her own
words:

> *"Last one. Everything I've got about moving on rock -- it's yours now.
> I'm getting old; these shoulders are shot. Won't be long 'fore the young
> ones are watching you climb. So pass it on, hey? That's the whole deal."*

**She turns up reliably** -- 72% of days once the arc is live, measured at
better than 120 days in 200. A mentor you have to farm is a spawn, not a
person. And **she is done when she is done**: five lessons and she goes back
to the boulders, because an arc that can be farmed forever is a training menu
with a face on it.

`TUT-6` is the fix for the hole in her own design: she was crag-only and
gated on V3, **so a brand-new player never met her at all** -- the character
who exists to teach you the game was unreachable until after you had worked
it out. She is in the Lot from day one now, saying one true thing per visit
about your actual situation (broke, no rig, starting to climb something),
each once ever, and her last line hands you to the arc. Said on arrival
rather than behind a key: an old-timer you have to press a button to hear is
a tooltip.

## The half where you teach

Three standing clients, each a named person with a strength and a weakness.
**You set the plan, and that is the whole mechanic** -- coaching somebody on
their weakness is meaningfully better than a comfortable hour on what they
are already good at.

They start on their **strength**, deliberately. You do not know somebody's
weakness on day one; that is what the first session is for, and starting them
on the right answer would make the plan a formality.

Progress carries, and a client who gains four grades **graduates** -- a
parting gift, your name moves, and the slot frees up. The payoff is that it
ends.

`COACH-5` is what makes it real rather than a number in a panel: **your
people show up.** A client who knows you turns up to the league night you are
running, how they do is written by who they are -- their strength carries
them, their weakness is where it goes sideways -- and it lands on your
standing, because everybody in the room knows whose client that is.

## Two things measurement changed

**The craft term was drowning the plan, eight to one.** The source weights
coaching craft at 0.02 a point against fit's 0.75 -- and its coaching craft
runs **0..12**. This port's runs **0..100**, so the number as written is
worth 2.0 and pins quality at its cap for anybody past craft 13. The plan
becomes a label and coaching somebody on their weakness stops meaning
anything.

**The test is what caught it**, because it asserted the thing the system is
*for* rather than the numbers it produces: weakness beats strength beats
generic. Fifth re-derivation of this kind and the first that is a *scale*
rather than a year length -- `GYM-6`, `GYM-8`, `TAX-1`, `DEPTH-6` and now
this. The shape is always the same: a number tuned against a range this port
measures differently.

**And `JOB-10` was a bonus with a nice name on it.** Rolled per session, a
thirty-year career produced **1,647 prodigies in 5,475 sessions**. Three in
ten hours cannot be a revelation. It is a fact about the *person* now --
rolled when you take them on, paid **once**, on the session where you finally
read them right. The talent was theirs before you met them; the seeing is
yours. Re-measured: **66 across a career**, one in four of the people who
came through.

That flag is also the one thing in this port that crosses the engine mirror
without being a `UPROPERTY`. It has to survive the round trip -- dropping it
would re-roll a client's talent on every call -- and no Blueprint may read
it, because it is the answer to the question the coaching is asking. A plain
member does both.

## What a coaching career comes to

Thirty years, three sessions a week, at the top of the craft:

| | sessions | graduated | earned | rep |
|---|---|---|---|---|
| reads them right | 5,475 | **234** | $153,555 | 3,486 |
| takes the easy hour | 5,475 | 195 | $148,575 | 2,820 |

**Coaching well pays about the same and produces different people.** +20%
through the door and +3% in the pocket, because you are paid by the hour
either way -- what changes is whether you were any good. That is the honest
shape of the job and it is a better one than a cash bonus would have been.

## What is Evan's

**The books are at the van, on `Q`.** It opens them, cycles whose hour it is,
and takes somebody new on when it wraps round; while they are open the number
keys set that client's plan, borrowing the pattern the guidebook already uses.
`E` gives them the hour.

**It wants a real panel and it does not have one.** Five plans against three
clients is exactly the shape that needs a list on screen rather than a toast
per press -- `RosterLine`, `CoachWhyNot`, `FocusName`/`FocusTeaches`/
`FocusBecomes` are all wired and all say their piece in one line each. That
is a UMG job and it is the same text box `notes/phase2-naming-widget.md`
already specifies.

**One thing measured and left alone.** 234 graduates over thirty years means
the eight-person cast cycles about thirty times, so Priya comes back. The
source calls them *archetypes* rather than individuals and that is a
defensible reading, but it does mean the same eight names recur for a
career. Whether they should be recycled archetypes or eight people who are
gone once they graduate is a design call against the source, so it is written
down rather than taken.

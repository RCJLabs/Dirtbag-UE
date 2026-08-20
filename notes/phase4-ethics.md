# Ethics: the things you did, and the day somebody finds out

The brief was *"multi-beat arcs whose consequences resurface seasons later"*
(`concepts/DIRTBAG.md`). That last clause is the whole design, and it is what
separates this from the faction system — factions already price the things
you do openly, instantly, the day you do them. This is the other kind of act:
the one nobody sees.

So an ethical shortcut is not a transaction. It is a **secret with a fuse**.
You take the benefit now — the line goes, the grade is yours, the sponsor is
happy — and you carry a thing that can come out.

## The mechanism

> **The more people are watching you, the more likely it is that somebody
> noticed.**

Discovery scales with visibility, and visibility is Scene standing plus
whatever your sponsor has made of you. Measured, chipping one hold:

| who you are | median years to discovery | never, over 30 years |
|---|---|---|
| a nobody | 12.6 | **50%** |
| known at the Lot | 9.0 | 15% |
| a name, gear sponsor | 6.4 | 4% |
| a star, title sponsor | **5.2** | 2% |

A quiet career has better than even odds of carrying it to the end — which is
the whole reason anybody would do it, and the thing that makes this a
temptation rather than a trap with a timer on it. A star does not: caught
around five years in, which is late enough that the act was made by somebody
younger and the bill lands on the career they built instead.

**Success is what exposes you.** That is both true and the only version of
this worth playing.

The first pass had the fuse far too short — 98% of nobodies caught, and a
star found out in six months. A fuse that short is not an arc.

## The five acts, and what survives them

| act | what it buys | does the ascent survive? |
|---|---|---|
| chipped a hold | the line goes | **yes** — hollow, on rock that is not what it was |
| retro-bolted | somebody's ground-up line, made safe | **yes** |
| claimed a send | the grade, without the send | no |
| staged a photo | the sponsor is happy | no |
| pulled on gear | the send | no |

Chipping and retro-bolting change the rock; the ascent stands, hollow, on a
line that is not what it was. The other three are lies about *what happened*,
and there is nothing left to stand. When one of those comes out the send is
stripped from the record — which is what gives the system teeth, because your
hardest send vanishing cascades straight into a sponsor's next review.

The old guard is the injured party: these are their ethics and the rock is
theirs. The Scene is not innocent but it is not injured either — it punishes
being *caught*, at half the rate. Only the two acts done to rock reach the
stewards.

One at a time, always. A career unravelling in a single afternoon is a
punishment; this is meant to be a story.

## A test that passed for the wrong reason

`TestAFreshSecretIsQuiet` ran four hundred iterations against a single world
seed. Discovery derives its stream from `(world, day, secret index)`, so all
four hundred were rolling the *same* forty-five numbers — it looked like four
hundred samples and was forty-five. It passed with the quiet-period guard
deleted.

Caught by injecting that exact defect. The world seed varies per iteration
now, and the reason is in the test so nobody collapses it back.

## Wiring

Discovery is called from `Sleep()`, and lands in `EthicsNews` beside
`DogWorry` and `VanNews` — a secret should reach the player the way it
reaches a real climber, which is that you wake up and everyone already knows.

It lives there rather than being a call the day loop remembers because
**five** separate per-day ticks have now been written and left uncalled in
this project: `KitDay`, the body roll, `FactionDay`, and the legacy save
path. Anything that happens overnight happens in `Sleep`.

## Still unwired, deliberately

Nothing offers the player these acts yet. There is no "chip it" verb on a
wall and no sponsor asking for a staged shot — those are decisions that need
a UI to be decisions at all, and a keypress that silently chips a hold would
be worse than not having the system. `DoSomethingYouWouldNotAdmitTo` is the
entry point when there is a screen to put it behind.

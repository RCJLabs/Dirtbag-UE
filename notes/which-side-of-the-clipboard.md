# Which side of the clipboard

*2026-08-25*

`Sim/DirtbagGym.{h,cpp}` extended, SAVE v45. `GYM-9` ported from the 2D
source.

For the whole game the circuit has met somewhere else. There are three
climbing gyms in this town and the two that matter belong to somebody else,
which is a fact the port has modelled since `GYM-4` and never let the player
do anything about. This is the thing you can do about it.

Two decisions, and they are different in kind.

## The bid, which is a decision because it costs something

You put your gym forward and the federation answers on the spot. It is not
weighing your feelings about your building; it is weighing **your walls,
your equipment and your membership against Send City's and The Cave's**, and
those two are moving targets — the same `RivalPull` the membership drift
already divides by, so a season spent building the place up is a season
spent on the bid.

Three gates before you can ask: forty-five members (*"a room that can hold a
circuit round"*), the first rung of the equipment ladder (*"walls somebody
would put a national number on"*), and $2,200 of deposit, insurance rider
and sanctioning fee.

**The deposit is gone either way, and a rejection sticks until next season.**
That is the whole reason the bid is a decision: without both halves it is a
re-roll you buy until it lands.

`GYM-12`'s wing table gets its `bid` column back — the other half of the
pair cut in pass two for want of a reader. A back room with a steep board in
it is what says somebody serious trains here.

## The round, which is a decision because it costs the other thing

On the day: you can run it, or you can climb it. **The host does not get a
scorecard.** Nobody who has ever set a comp they were entered in would
pretend otherwise — you know where every hold is.

Running it pays thirty-four entries at $26 a head, brings nine people who
came to watch and came back on Tuesday, costs six hours and eighteen energy,
and the season's points go to somebody else.

`Circuit` gained `HostedIt` for it, beside `Forfeit`, and the difference
between those two is the point. A forfeit costs you the points *and* goes on
the ranking record as a result worth less than nothing, because you were not
there. Hosting costs you the points and **nothing else**, because you were
there all day with a clipboard, and nobody thinks less of a host.

## The bug the measurement found

`CompIsToday` has always meant *is there a comp date today*, and until now
that was the same question as *is there a comp for me today*, because there
was exactly one thing a comp date could be spent on.

`GYM-9` gives a comp date a second use, and the first measurement showed it
immediately: a career hosting **457 rounds** while also entering **740
comps** out of a possible 780. It was running the round in the morning and
climbing the round it ran in the afternoon.

So `RoundIsOpen` — a date on the schedule that nothing has yet resolved —
and every caller that meant "is there one left for me" now asks that
instead: the day verb, the probe, and the engine's own `CompIsToday()`,
which the climb wall reads twice. A predicate that cannot express "spent"
cannot express this system at all.

## What it measures out at

Thirty years, six seeds, and the two ends of the trade measured separately
because no single policy can sit in the middle of it:

| | a hoarder who hosts | a competitor who owns the room |
|---|---|---|
| seasons bid for | 156 | 156 |
| seasons won | **99** (63%) | 99 |
| rounds actually run | **457** | **9** |
| spent on deposits | $343,200 | $343,200 |
| taken in entries | **$404,283** | $7,956 |

**Bidding is worth exactly what you do with it.** Run the rounds and it is
about **+$61,000 across thirty years**, plus the membership the crowd leaves
behind — a business that pays for itself and a little more. Bid out of habit
and never use the season, and it is a third of a million dollars for nine
days' work.

And a policy that competes *will* climb the round, every single time it can:
740 of 749. That is the honest shape of the conflict and it is why the
system needs the bid to cost something up front — the decision that matters
is made a season early, when you choose to ask.

## Two things worth Evan's eye

**The deposit was priced against a shorter game.** A thirty-year career here
sees about **156 circuit seasons**, so bidding every one of them costs more
than thirteen gyms. That is fine when the player bids deliberately, and it
is what makes the naive policy's $343,200 the informative number it is —
but the price was set in a game whose careers are a few hundred days long,
and it has never been checked against this one.

**`savings=` was a silent no-op on every policy but the hoarder**, which is
why this system was unmeasurable until now: nothing in the battery both
competed and could hold the $25,000 a building costs, so the one decision
`GYM-9` is *about* had nobody who could face it. Asking for a savings target
is asking to hoard, on any policy, and `never-happened.py` has the
competitor-who-owns-the-room row now.

## What that leaves

`GYM-10` — the league you run, whose format is not cosmetic because it
decides who wins: a board ladder belongs to whoever trains, a handicap
league to whoever improved, a team league to whoever turns up with people,
and every one of the thirteen regulars leans one way. It is the last of the
family, and the only one that reads the cast `GYM-2` and `GYM-5` built.

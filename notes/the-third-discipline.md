# The third discipline

*2026-09-01*

`Sim/DirtbagSpeed.{h,cpp}` new, SAVE v47. `SPEED-1` through `SPEED-4` and
`OLY-4` ported from the 2D source.

The Games have been in this port since `Sim/DirtbagWorldStage.h` and have run
on two disciplines. That is not the format. An Olympic climbing programme is
boulder, lead and **speed**, and the audit found it as the largest single
hole in a system that is otherwise finished -- which is the worst kind of
gap, because a system that is 90% ported reads as a bug where one that is 0%
ported reads as a plan.

## Why it is a minigame and not a roll

The source's own line is the brief: *"the whole event is the reaction start:
wait for GO, then go."* Nothing about a speed run is a question of whether
you can do the moves. Everyone in the final can do the moves; they have all
done this exact route ten thousand times. It is a start and a cadence.

So this is the first thing in the port since HOLD TO CLIMB that the player
actually **plays** rather than watches the sim arbitrate, and it takes the
shape `Sim/DirtbagSessionLoop.h` already established: a live core the
presentation layer drives, and a bot on top of it for every run nobody is
playing. `RunTime` is the whole clock and it takes **what the player did**,
not what the player is -- a false start, a reaction, sixteen rungs of
cumulative cadence error, and the reaches you fumbled.

Everything else reads off that one function, which is why a career resolved
headlessly and a career played by hand cannot drift apart.

## Three things the model gets right by construction

**The window is free.** Only cadence error *past* the window costs anything.
Sixteen reaches each a hair late is a clean run, and a model that charged for
them would make a perfect run the only clean one -- which is not what a
window is for.

**The floor is the wall record and it holds against everything.** The route
is the same route for everybody and it has a bottom. No grade, no reaction
and no cadence gets under 4.6s, which is also why the reaction bonus is
capped: without the cap a superhuman start beats the world record, and the
record is the point of the floor.

**A DNF is a time, not a flag.** Thirty seconds, which scores zero by
construction. That means a heat can compare two blown runs without a special
case, and the tie goes to the opponent the same way every dead heat in this
game does.

## What measurement found

**The clock lands where a game scale should.** A grade-12 climber -- the top
of what `SkillToGrade` produces from a maxed skill sheet -- runs a mean 7.7s
with a 6.9s best. A grade-4 climber runs 13.3s. The floor never binds in
practice, which was worth checking: it binds at grade 16.3 and nothing in
this game reaches that.

**Nerve buys the start and nothing else.** A steady climber goes on the amber
1.7--2.5% of runs against a rattled one's 8--9%. It does not make you climb
the wall better, because everybody in the lane has the wall memorised; what
a crowd takes from you is the tenth of a second at the buzzer.

**And the heat is a lottery in a way the time trial is not.** This is the
number worth knowing. Over four thousand runs a grade apart, the stronger
climber is ahead **90.0%** of the time on the clock. Put the same two people
in one race and it is **85.6%** -- and against their own grade it is
**49.6%**, dead level. The opponent's heat spread is 1.4 seconds wide, which
is two grades of pace.

That is the source's design and not a slack dial. Its own comment says the
quals *"seed you"* and the final is *"win-or-go-home"*, and a knockout that
the seeding decided would not be a knockout. Run out over two thousand
brackets, a grade-12 climber against a field seeded 13.0 down to 10.2 takes
**gold 4.5%, silver 25.1%, bronze 22.9%, fourth 19.9%, and goes out at the
quarterfinal 27.6%** -- because the final is always against the top seed, who
is a grade above you, and the quarter is one race against somebody only
half a grade below.

## The one deliberate deviation, and it is about the ranking

`OLY-4` splits the national ranking three ways, so **which room you are
allowed into is read off your ranking in that discipline** -- a National
boulderer walking up to a speed wall for the first time is a Local speed
climber, and should be entered as one. That is ported exactly, as
`RankingIn`.

The source then makes `natlPts` the **best** of the three. This port keeps
its own sum, and the reason is that the two rankings are different objects.
2D's is a **lifetime total**, where a best-of-three is the only thing
stopping three disciplines inflating one figure. This port's has been a
**one-year rolling window** since the ladder shipped, and its three named
rungs -- 700 for the national team, 1200 for the Games, 2200 for World-Class
-- were measured against the sum. Taking the max would cut every existing
career's ranking by roughly two thirds against thresholds nothing else moved,
which is three systems re-tuned to import a fix for a problem this port does
not have.

Logged here rather than improvised, per CLAUDE.md.

## And a save that knows what it did not know

`RankingResult` gained a discipline, so the v46 migration has to answer what
discipline ten years of existing results were. **All three.** A v46 career
has no record of it, and tagging them `Boulder` would tell a National lead
climber they have never entered a lead comp. The 2D game's own migration
makes the same call for the same reason (`discRank = {natlPts, natlPts,
natlPts}`), and the flag that says so is on the result rather than inferred
from the version, because a save is loaded once and read forever.

## What is not built, and is Evan's

**The wall has no door yet.** Every verb is wired to Blueprint through
`UDirtbagSimLibrary` and the sim is complete and tested, but a speed wall is
a *place* -- and placing one is Phase 6 work that cannot be done from a
container. The checklist is in `notes/NEXT-AT-THE-DESK.md`.

Deliberately not here: the reaction-start and cadence widgets themselves,
which are the presentation layer's, and which is exactly the split the whole
project runs on.

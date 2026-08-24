# The narrator — Phase 5, from the side a container can reach

*2026-08-24. Evan asked what was left that was not asset work; this was the
recommendation and this is the build.*

`DIRTBAG.md` §9 names the project's top risk as **"session isn't tense or
legible in 3D"**, and Phase 5 calls staging the craft centre. Most of that
needs an Editor. This part does not.

## The hole

The resolver has always returned a full per-move timeline — the odds each
move faced, the pump after it, whether it stuck, and since yesterday what
was on the rope while it happened. **Nothing anywhere turned that into
words.** There were fragments (`HowCloseText`, `RunoutText`, `PieceText`,
`BelayText`) and no layer that read a whole attempt and said what happened,
so a twenty-move trad pitch and a six-move boulder produced the same
silence.

## The rule

**The narrator's job is knowing when to shut up.**

A line per move is a log, not commentary — twenty lines for a pitch is
twenty lines nobody reads, and the moment that mattered is somewhere in the
middle of them. So a beat is emitted only when something *happened*: the
crux arriving and not being certain, a move that had no business staying on,
a shake-out that bought the route back, the forearms crossing over, getting
off the deck, a piece that went in badly, the hands opening.

Two rules it does not break:

- **It reads; it never recomputes.** Every beat is derived from the result
  the sim already produced. If the narrator wants something the result does
  not carry, that is a signal the *result* should carry it — which is
  exactly how `AttemptResult::gear` came to exist for trad. A narrator that
  recomputed odds would be a second opinion about what happened, and the
  first thing it disagreed with would be a bug nobody could find. A test
  hands it a timeline the resolver never produced and it agrees with it.
- **Never a number.** The standing rule for text here, and `HowCloseText`
  says why: *"you fell at move nine of twelve"* is a thing you read; *"one
  move, that was the go"* is a thing you see. A test sweeps three
  disciplines × every highpoint × three odds × three pump rates and fails on
  a digit.

## What it produces

Ten kinds of beat, each with a `weight` so the presentation can pick — a HUD
takes the loudest three, a camera pushes in past a threshold, a replay shows
the lot — and one sentence for the career's history.

Real output, unedited:

```
=== SPORT PITCH (11 of 16 moves) ===
  [0.30] ground       Your grade, on a good day.
  [0.38] off the deck Clipped. The ground stops being the question now.
  [0.50] shake        Shakes it out. Some of it comes back.
  [1.00] fell         Off pulling up the rope. That is the classic and it
                      never stops being infuriating.
  --> LOG: Blew the clip.

=== TRAD LEAD, NUTS (15 of 18 moves) ===
  [0.30] ground       This should go.
  [0.38] off the deck First piece in. It will probably hold, and it is
                      better than the ground.
  [0.43] shake        Shakes it out. Some of it comes back.
  [0.55] runout       The gear is below your feet. Whatever happens next,
                      happens.
  [0.84] crux         This is the one.
  [1.00] fell         Off at the crux.
  --> LOG: Off at the crux. You had been a long way above the gear for a while.
```

A career's log for those nine sample attempts reads: *One move short. /
Onsighted it, the crux went and it was not certain. / Never in hand. /
Flashed it, the rest in the middle is what did it. / Blew the clip. / Off at
the crux, you had been a long way above the gear for a while.*

## The four things writing it found

**1. It narrated every move on a route where every move was desperate.**

The first version emitted *"that should not have stayed on"* whenever a move
stuck against bad odds. On a pitch the climber fought for the whole way that
is **sixteen beats on a sixteen-move pitch** — not commentary, a log with
adjectives. A climber fighting all the way up is not having sixteen moments;
they are having one long fight.

The fix is one rule and it generalised to three places: **the narrator only
speaks when something beats everything before it.** The first shocker is a
beat and after that only a worse one is; the first good rest and after that
only a better one; the first bad piece and after that only a worse one. On a
uniformly hard route that is exactly once, and on a route with a sting in
the tail it is exactly twice, in the right places. As a bonus, a trad
leader's gear getting *worse* as they get pumped is now the story, which is
what the model actually does.

**2. The live wall and the replay disagreed about a top-out.**

`LiveAttempt::partial` is an `AttemptResult`, so the wall calls the same
function move by move — one implementation on purpose, because this project
has twice shipped two paths that drifted. It drifted anyway, in the one
place the partial is not the finished thing: **the style is a judgement
`FinishAttempt` makes.** Reading the ending off the highpoint, the wall said
a bare *"Top."* while the replay of the identical climb said *"first go, no
idea what was coming, and it went — that is an onsight and they do not come
back."*

The ending is read off `sent` now, and the asymmetry is the honest one: **a
fall is known the instant it happens; a top-out is not known until the
attempt is finished.**

**3. A trad lead was being told in bolts.** `RunoutText` says *bolt* —
correct for the HUD it was written for, and wrong out loud on a lead where
the whole point is that nobody drilled anything. The narrator writes its own
runout line now. One vocabulary per voice.

**4. The reason for the fall was the fall.** Picking the loudest beat as the
*why* produced *"Off at the crux."* and nothing after it, because the crux
beat outranked the runout that actually explained the afternoon and then
declined to say anything about itself. The beat you fell on is excluded:
what happened at the moment it ended is the ending, not the reason for it.

That fourth one **had no test until the reintroduction pass found it** —
the fix was already in and putting the bug back changed nothing that failed.
Written now.

## Where it plugs in

- `HowItWent` runs in `ApplyAttemptToDay`, the one function every burn in
  the game passes through, and lands on `DayState::lastBurn`. Not saved: a
  burn is a thing you read once and then climb again, and a career's history
  is the project ledger's job.
- `UDirtbagLiveAttempt::LastWord()` is the wall's door — an empty line means
  silence, which is the right answer most moves.
- `UDirtbagLiveAttempt::Beats()` is the replay's, and it tells a finished
  attempt from the finished result for the reason in §2.
- `Loudest` goes back through the sim from Blueprint rather than being
  re-sorted in engine code, because the tie-break that puts an ending last
  among beats on the same move is a rule.

## On screen

*Added 2026-08-24.* Same rule as the habits readout: **which surface a fact
goes on is decided by how long it is true for.**

- **A beat is a moment**, so the wall toasts it — `LastWord` after every
  `StepMove`, on one slot so the newest replaces the last rather than
  stacking. Coloured off the `BeatKind` rather than off the words, which is
  what that enum is for: restaging a beat must never mean re-reading it. And
  **held for a length that scales with `weight`** — the moment of an attempt
  stays up twice as long as a shake-out, which is the one place that number
  does a job in the engine.
- **The last go is true until the next one**, which is most of a session, so
  `HowItWent` sits on the session readout beside the attempt number — the
  same reasoning that put "attempt 14" there instead of in a four-second
  flash. A toast would have taken it away while you were still looking at
  the move that did it.

Two things it replaced rather than added to:

- **`"shake  -14 pump"`** was a stat line where a sentence belongs, and it
  had been on the wall since Phase 0. The shake shows up in the timeline as
  pump going *down* across a move — the only way it can — so the narrator
  sees it without being told and says whether that was a chalk-up or the
  rest that gave you the route back. The wall keeps its own line for a shake
  that *cost* you, because that one is genuinely invisible to the timeline:
  pump going up across a move is indistinguishable from the move having been
  expensive.
- **The end-of-attempt toast** led with `HowCloseText` and followed with the
  count. Both halves of that reasoning are still right, and the narrator now
  says the sentence twice already — once loud at the moment it happened and
  once persistently on the readout — so a third telling a beat later in
  different words is **one screen disagreeing with itself about what just
  happened**. It keeps the job nothing else does: the count, for the player
  who wants it.

The send toast is untouched and does not duplicate anything: it says the
route, the grade and the style, and the readout says *"Onsighted it."* The
top-out beat deliberately does not fire live — the style is a judgement
`FinishAttempt` makes — so the loud moment on a send is the animation, and
the sentence arrives with the commit.

## Still open

- **No sound and no camera.** The `BeatKind` enum exists so both can key off
  a beat rather than off its words; the toast colour does, and neither of
  the other two does yet.
- **`Beats()` has no reader.** The replay door is open and nothing walks
  through it — the highlight reel is a Blueprint job.
- **A replay says less than a driven attempt.** Beats fire from `CommitMove`,
  which only the live path calls, so a watched bot attempt gets the readout
  sentence and none of the moments. Correct for now (a replay is a replay),
  and the first thing to revisit if watching somebody else climb ever
  becomes a feature rather than a fallback.
- The narrator says nothing about the *belayer*, the partner, or the crowd
  at a comp. All three have text of their own elsewhere; none of it is on
  this timeline.

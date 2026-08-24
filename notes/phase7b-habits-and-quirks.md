# Habits and quirks — Phase 7's second pass

*2026-08-24. Queued by Evan behind trad; the back half of the character
system Phase 7 deliberately left out and said why:*

> *"Quirks, habits and personality drift are all earned from a tally of how
> you actually climb, which needs instrumentation in every session and is a
> coherent second pass."*

## The rule the whole thing turns on

**A habit is what you have been doing lately, and it can change.
A quirk is what you turned out to be, and it does not.**

Everything else follows. A habit is *derived* from a decaying logbook and is
never stored as a fact — stop doing the thing and it goes, which is what
makes it a habit rather than a badge. A quirk is stored, permanent, and
earned exactly one way: by holding the habit for two seasons, until it
stopped being something you were doing and became something you are.

**The trap is the point.** The Grinder who hardens into Obsessive keeps the
beta bonus for the rest of their career *and keeps the head penalty too*,
long after they have stopped grinding. You do not get to un-become somebody.

## What is in it

Nine habits, each earned from a ratio in the logbook and each hardening into
one quirk:

| habit | earned from | what it does |
|---|---|---|
| dawn patrol | 55% of days out started early | endurance up — the approach is the training |
| grinding | 60% of burns on one line | beta 1.45×, technique up |
| ticking | 42% of burns are a line's first | beta 0.72×, head up |
| never warming up | 55% of burns at your limit | power up, injury risk 1.22× |
| indoors | 62% of days out are plastic | fingers up, head down |
| climbing on nothing | 35% of burns on gone tips | skin cost 0.82× |
| knowing when to stop | 30% of days out ended with skin left | risk down, head down, nerve down |
| running it out | 30% of trad moves with nothing on the rope | head up, risk up, nerve up |
| sewing it up | a piece every three trad moves | head down, risk down |

Plus six **picked** quirks at creation — a light sleeper, bad with money,
superstitious, stubborn, gregarious, quiet — each a small perk against a
small cost, in lanes no earned quirk touches. Picked and earned share an
enum, because by the time anybody is reading it back, *how* you came to be
like this is not the interesting part. `Pick` refuses an earned one:
choosing to be obsessive at the counter is not the same thing as becoming
it over two seasons, and that difference is the only rule this file has.

**Nothing here touches send odds.** Phase 7's rule is that only a flaw may,
so where you came from can never be a difficulty setting — and an earned
trait has even less business there, because a habit that made the moves
easier would be a game telling you how to play. These bend what a session
teaches, what it costs, what it risks, and how steady you are above the last
piece: seams that already exist and are already measured.

## What a career turns into

Three thirty-year careers, each run under three policies:

| seed | policy | sends | quirks | days with a habit | became |
|---|---|---|---|---|---|
| a | greedy | 25 | 3 | 582 | obsessive, leathery, cautious |
| a | trad | 11 | 2 | 1,453 | impatient, obsessive |
| a | kitted | 26 | 4 | 10,941 | impatient, obsessive, a morning person, a plastic merchant |
| b | greedy | 14 | 1 | 1,497 | obsessive |
| b | trad | 8 | 1 | 1,679 | obsessive |
| b | kitted | 21 | 4 | 10,942 | impatient, obsessive, a plastic merchant, a morning person |
| c | greedy | 23 | 1 | 1,435 | obsessive |
| c | trad | 8 | 2 | 1,123 | bold, obsessive |
| c | kitted | 22 | 4 | 10,942 | impatient, obsessive, a plastic merchant, a morning person |

**Different policies produce different people**, which is the gate. The
climber who buys a gym membership has a habit on *every single day of a
thirty-year career* and ends up a plastic merchant and a morning person; the
one who only gets out when the rock is in condition holds a habit about one
day in eight and ends up with one or two. The trad leader is the only one
who ever becomes *bold*.

Everybody becomes **obsessive**, and that is honest rather than a bug: every
policy this probe has grinds one line, and the logbook is reading back what
the probe actually does.

## The bug the career measurement found

**The weather was giving people a personality.**

The first version counted `Bailed` — a session that started and produced no
burns — as backing off, and read the Cautious quirk off it. Measured over
thirty years, that turned out to mean *the rock was never in condition*: one
career's last season came out **71 days out and zero burns, every one of
them counted as bailing**, so `bailed` sat at 1.00, every career in the game
ended up cautious, and the thing that made them cautious was rain.

Two changes:

- **A day out is a day you got on something.** A session that started and
  produced nothing is not a day out at all. It is not a bail either — it is
  a forecast.
- **The signal is `StoppedEarly`**: you went home with skin still on your
  tips, read against the same thin-skin line the session advice uses to say
  *"your tips are gone; jugs or go home"*. Stopping while you still have
  something is a decision. Not getting on because it rained for a fortnight
  is a fortnight.

Before the fix, every career in the table above ended cautious and had a
habit on 10,936 of 10,950 days. After it, `cautious` appears once in nine
careers and habit-days range from 582 to 10,942. **The system did not
discriminate at all until the signal was a decision.**

## Two smaller things

**`atGym` has been misnamed since Phase 1.** It means *a session started
today*, and both the wall and the crag set it. Nothing needed the
distinction until the logbook did, and inferring it from the padding would
have been a guess. `DayState::indoors` says it now, and only `GoToTheGym`
sets it.

**`thinSkin` was a mirror before it was a line of code.** The first draft put
it in `DayDials`; `SessionLoopDials` already owned it, and the dial checker
caught the duplicate within a minute of it being written. The day loop reads
the session advice's own number, because *"climbs on nothing"* and *"your
tips are gone"* have to mean the same thing.

And a name collision, the fourth in a fortnight: `Record` is already a
*function* in `DirtbagComp.h`. The logbook is called a logbook.

## Where it plugs in

- `Logbook` and `Quirks` live on `PlayerState`; `SAVE v34` carries both, and
  a v33 career loads as nobody — exactly neutral, so it plays with the
  numbers it was measured with.
- The **instrumentation** is in `ApplyAttemptToDay`, the one function every
  burn in the game passes through, and in `SleepToNextDay`, which is the only
  place that knows what kind of day it was.
- The **nerve** reaches an attempt through `BodyContext`, beside the
  temperament, because a habit is part of the body you climb in — and
  because that struct exists precisely so a second attempt path cannot be
  built that forgets half of one.
- **Beta and skin** reach a burn through `SessionState`, copied off the
  career at the start of the session exactly like the padding and the rack.
  On the session rather than passed down through `CommitAttempt` because
  that is the one place every path already meets — the batch loop, the live
  attempt the minigame drives, and the engine's own commit — and a parameter
  would have reached one of the three.
- Creation gained a fifth question, and it is the only place a picked quirk
  can ever come from.

## Still open

- **`becameToday` is carried and nothing says it out loud yet.** The sim
  produces the line (`QuirkLanded`) and the engine can read it; no HUD reads
  it. Same shape as `WorkedOn`'s returned talent, which has been discarded
  at its call site since Phase 7.
- **Personality drift** — the third thing Phase 7 named — is not here. The
  axes still hold whatever creation set them to. Habits are the observable
  half; drifting the axes themselves off the same logbook is a third pass.
- Every probe policy grinds, so `obsessive` is unanimous. A policy that
  spread its burns would be a better test of the Tourist half of that pair.

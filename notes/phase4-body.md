# Training load and injury: the second body budget

Built first of Phase 4, out of order and deliberately, because both Phase 3
measurements ended at the same wall. Skin caps the climbing year, so nothing
money could buy added climbing to a season — it only moved it around
(`notes/phase3-kit.md`). A crash pad was worth +37% sends precisely because
it spends *no* skin; the gym and the hangboard measured neutral because they
spend the same skin the crag does.

Load is the second budget, from the other side: one you cannot climb your way
out of, only rest or pay your way out of.

The model in one line: **hard climbing accrues load, load heals slowly, and
load past a threshold is how you get hurt.**

## The result

Twelve seasons per policy, 365 days, rest-until-skin 3.0.

| policy | sends | injuries | days hurt | physio |
|---|---|---|---|---|
| outdoors only, no board | 38 | **0** | 0 | 0 |
| board, stops at the warning | 38 | 3 | 55 | 9 |
| board, hangs regardless | **24** | 13 | **1,467** | 214 |

Three things, all of which survived twelve seeds:

**1. The weather protects you completely.** An outdoor climber at a
weather-limited crag cannot overtrain — 157 days a year never come good and
75 more go to resting skin, so they climb one day in three and load never
gets near the line. Zero injuries in twelve seasons. That is not a dial being
soft; it is a true thing about weather-limited crags, and it means the game
should never hand out an injury to somebody who only climbs outside.

**2. Buying your way past the weather is what gets you hurt.** A hangboard is
the one thing in the game that loads tendons without the weather getting a
say, and it is the single most reliable way real climbers get injured. Own
one and the protection is gone.

**3. The warning is worth 37% of your sends.** `LoadText` says "everything
aches; this is the warning" above the threshold. A player who stops training
there sends 38; one who keeps hanging sends 24, and spends 1,467 days hurt
against 55. That is the mechanic's whole value, and it is legible before it
bites rather than after.

## Design decisions worth keeping

- **Load takes the trainer's own "was that hard" number.** Two ways of asking
  that question would drift and the drift would be invisible until somebody
  got hurt for no reason.
- **An injury is specific, and decides what you can still climb on.** A
  pulley ends crimping and leaves slopers alone; a shoulder is the reverse; a
  lumbrical is pockets and only pockets; an elbow is everything, a little,
  which is why nobody rests one properly. Becoming a sloper climber for a
  month is the injury's actual gameplay.
- **Nobody gets hurt out of the blue.** Below the threshold the chance is
  flat zero, never small — an injury is always something you were warned
  about.
- **Tendons do not tear in a camp chair.** The roll only happens on a day you
  pulled on.
- **Climbing on it is a gamble both ways.** Most of the time you get away
  with it, which is what makes pulling on a decision rather than a warning
  label. Twenty burns are twenty separate gambles.
- **Physio is money for time**, rate-limited to once a week so a rich season
  cannot buy its way out of a bad one overnight. The reckless player spends
  around a quarter of their income on it — which is, at last, money buying
  climbing back rather than moving it around.

## Defects this measurement caught

- **The hangboard accrued no load at all.** The most injury-causing activity
  in climbing was free, and it was priced in *skin* — which was never what a
  board costs. That stand-in existed only because there was no tendon budget
  to charge. Skin cost dropped 1.6 → 0.3, load 0 → 14 (more than a four-burn
  session), and the board went from a thing that competed with the crag for
  the wrong resource to one that competes for the right one.
- **Load was asking the wrong question.** The first pass measured "how many
  grades over you was the route", so a player projecting *at their limit* was
  over by roughly zero and a whole season peaked at 6 out of 100. Climbing at
  your limit is the maximal intensity — that is what limit means.
- **Aggravation ran away.** Adding days per aggravation turned one bad
  fortnight into a 226-day injury. Severity is what rises now, and the days
  are recomputed from it, so the worst an injury can be is the worst an
  injury is. Climbing on it every day still means it never heals — which is
  correct, and is the player's own doing.
- **The probe's table had silently empty columns, twice.** A field was
  appended to the printf's values and not to its format string. Every
  machine-readable line is now preceded by a `HEAD` row emitted from the same
  place, so a column can never again be read off against the wrong name.

## What it says next

Two signatures changed to make a class of mistake impossible rather than
merely unlikely: `SleepToNextDay` and `ApplyAttemptToDay` now take the world
RNG as a *required* parameter, because those are the two places the body
rolls. A caller who forgot would have played a game where nobody ever got
hurt and nothing ever cost anything — and with no Unreal compiler in this
loop, silent absence is the failure mode that actually happens here.

Still open from Phase 3: the second crash pad is the only purchase measured
to move the outcome, and the probe has never once afforded it. Load does not
change that — but physio is now a second thing money buys that hands climbing
back, and it is the first one a player will actually reach.

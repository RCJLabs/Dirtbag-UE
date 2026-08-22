# The handover

*2026-08-22. Phase 6.*

Evan: *"what else can we build without the editor"* — so I audited instead
of guessing, and the audit turned up something worse than a gap.

## You could not end a career

`RetireAndPassItOn`, `TimeToThinkAboutIt`, `CareerEpitaph`,
`GenerationsBefore` and `InheritedGuidebook` are all `BlueprintCallable` or
`BlueprintPure`, and **nothing called any of them.** No C++ caller, no
Blueprint.

Phase 4's headline system — legacy and generations, measured across ninety
years and four generations in the harness, with inheritance rules, a save
migration, crew names and dreams carrying over — worked perfectly and could
not be performed.

That is the **written-and-never-wired** bug this project has now found at
five layers, and the largest instance of it. The others were a system
without a verb; this is a whole phase without a door. The reachability
checker cannot see it: it proves *sim* declarations are reachable from the
engine, not that engine functions are reachable from play.

It also unblocks a gate. Phase 4 asks that *"a career reads like somebody
lived there"*, which needs a career **ending in a handover** — and the only
evidence has been headless because the handover could not be performed at
the desk.

## The second hole, found on the way in

**There was no way to name a climber at all.** `ClimberName` is an
`EditAnywhere` string on the game instance with no in-game setter anywhere,
so:

- every first ascent in the game was signed **"you"** (`AscentSignature`
  falls back to it), and
- the crew hash — which was given the player's name *specifically* so four
  generations would stop being one generation four times — was being fed an
  empty string.

That fix was measured and logged three days ago. It has never actually
worked in play.

## What it is

**R at the van**, on its own key and with its own confirm.

Its own key because E at the van repairs it, and a career should never end
because somebody meant to fix a wheel bearing. Its own confirm because it is
the single most irreversible thing in this game — no undo, and
`RetireAndPassItOn` writes the save immediately. Walking away disarms it, so
a confirm cannot sit armed across half a season.

The van prompt is silent about stopping until the game has something honest
to say. `TimeToThinkAboutIt` is never a command and never age alone — it is
a run of injuries, or two grades off your best and past the age. **A line
offering retirement every night of a career is a line you learn to stop
reading.**

Then three beats:

1. **The epitaph.** Generation, and the career in one paragraph — hand-
   wrapped, because the canvas draws a string as one line however long it is
   and twenty years running off the right edge of the screen is a poor way
   to end them.
2. **Somebody turns up.** Three names, 1/2/3.
3. **Who you are now**, and **IN THE BOOK** — the lines earlier careers put
   up, with the one that just ended newly among them.

## Naming is a pick, not a text box

Typing needs a widget, an editor and a keyboard-focus fight. Picking needs
three keys the player already uses for the dream and the stake.

And it reads better: you are not naming yourself, **you are meeting the
person who rolled into the Lot**, which is what a handover actually is. The
same screen names a fresh unnamed climber — no confirm there, because
putting a name to yourself is not irreversible the way stopping is, and a
new career pressing R almost certainly means *who am I* rather than *I am
done*.

`ThreeWhoCouldTurnUp` draws from the world and the generation, so a reload
offers the same three — the no-reroll rule every gamble here lives under —
and generation four is offered different people from generation two.

## Three bugs caught, two of them by tests

**The pool had Dev and Bo in it.** Both are Lot regulars. The test that
checks no candidate shares a name with anybody at the Lot failed
immediately, which is exactly what it was for: the handover screen would
have introduced you to somebody standing behind it. The check stays in the
test rather than becoming a filter in the pool, deliberately — a filter
would silently absorb a name added to either list later; a test fails and
says which one.

**The book was backwards.** The first version showed `InheritedGuidebook()`
under the epitaph — but that walks the *filed* careers, so before retiring
it lists your **predecessors'** lines under your own obituary. It is asked
after the handover now, at the arrival, where it means what its name says:
the book the next climber opens, with you newly in it. That is the better
beat as well as the correct one.

**The van prompt came out as a box.** It gained a second line, and the
canvas draws a string as one line however many newlines are in it, so the
retirement offer would have rendered as a glyph. `PushPrompt` splits on
newlines into separate prompt lines now, which any prompt can use.

## What is deliberately not built

**Choosing between three *successors* rather than three names.** `Inherit`
already rolls skills and morphology, so offering three whole people — each
with a different body the resolver reads differently — is a small change and
a genuinely better moment.

It is also a design decision rather than a wiring job, and this was a wiring
job. Recorded here rather than done unasked.

## At the desk

Nothing to place. This is the one you should try first, because it is the
gate:

1. **Start a fresh career, walk to the van, press R.** Three names. Pick
   one. Every first ascent from now on is signed with it instead of "you".
2. **Play a career.** When the body starts going, the van prompt will say
   you have been thinking about stopping. It will not say it before then.
3. **Press R twice.** Read the epitaph. Hand it on. Meet whoever turns up.
4. **Then look at the book on the arrival screen** — the lines you named are
   in it, and the climber reading them never met you.

That fourth step is Phase 4's gate: *does a career read like somebody lived
there?* It has never been answerable at the desk until now.

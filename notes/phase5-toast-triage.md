# Toast triage

*2026-08-22. Phase 5, item 2.*

Forty-nine on-screen messages across two files — twenty on the climb wall,
twenty-nine at the day spots — all going through the same four-second
`AddOnScreenDebugMessage`. The phase brief said they are not one thing, and
sorting them needs exactly one question:

> **Is there a decision pending on this?**

News — *you ate*, *the van broke*, *somebody took your line* — is already
true by the time you read it and there is nothing left to do about it.
A message that expires is precisely right for that.

But *"Shoes? (E) — $180"* is not news. It is the interaction. It was said
once, four seconds long, while the player stood in the trigger with the
decision unmade — and then the screen went blank and the key still worked.

## The count

| | before | after |
|---|---|---|
| climb wall | 20 | 14 |
| day spots | 29 | 28 |
| **total** | **49** | **42** |

Seven messages left the toast layer entirely and two more were reclassified
as level-setup logs. **Forty-two stayed, and that is the point** — the
triage was not a campaign against toasts. Most of these messages are news
and the toast is the right shape for them.

## What moved, and why

### 1. The standing prompt

Everything that says *what a key does right now* is now a persistent prompt
panel along the bottom of the screen, drawn for as long as it is true:

- the day spot's `PromptText()` — every spot, every kind
- the wall's route line (name, grade, stars, the read, "E to climb")
- who is holding the rope, and how many burns they have left in them
- where the body is, when it is not Ready
- *"Press E to go again"* after a session

The day spot's prompt had a second problem underneath the first: **it was
said once on entry and never rebuilt.** Buy the shoes and the shop still had
a four-second-old sentence about shoes on it, and nothing at all about the
pad it would now sell you. It is rebuilt on every state change now, from one
call at the single exit of `OnInteract`, because a prompt refreshed in eight
branches is a prompt stale in the ninth.

The prompt owns the bottom of the screen and the session bar and the fire
table stack on top of it, so a three-line wall prompt cannot end up drawn
underneath the pump bar.

**Three tones and no more**: plain is what is here, good is worth crossing a
valley for (an unclimbed line), blocked is in your way (no belayer, no van,
a wrecked body). A prompt that colours every clause is a prompt nobody
reads.

### 2. A live bug the sorting found

The wall's belayer line and its body-state advice were pushed through the
**same keyed toast slot**, two statements apart in the same function:

```cpp
Toast(... Game->BelayLine() ...,  5.f, kToastResult);   // line 266
Toast(Game->SessionAdviceText(),  5.f, kToastResult);   // line 279
```

A stable key replaces rather than stacks — which is what those keys are for
— so on a roped line at the gym when you were tired, **the second silently
ate the first and you were never told who was holding the rope.** That is
the single most valuable sentence the wall says, it is the first thing in
the game that rapport buys and nothing else can, and it was invisible in
exactly the situation it mattered.

Separate prompt lines cannot do that to each other. `kToastPrompt` and
`kToastResult` are both gone; their numbers are left unclaimed rather than
reused, because two kinds of message sharing one slot is what caused this.

### 3. The session's own verbs

**"Attempt 14"** was a four-second yellow flash at the start of a go. Which
go this is on the same problem is most of what a session *feels* like and it
is true for all of it — it sits beside the route on the session panel now.

**The control reminder** (*"HOLD Space to load the move…"*) was a six-second
toast fired at the top of the route, which is the one moment a first-time
player is watching the climber rather than reading text. It is under the
grip bar it describes now, and it goes away at the first successful latch.

That flag is deliberately **not** per-attempt. The first version reset it in
`StartAttempt`, which would have printed the tutorial line at the top of
every go for thirty in-game years — the toast's problem with extra steps. It
lives on the game instance as `bLearnedTheVerb`, set once at the first latch
and never unset, and is republished into the readout every frame so the
end-of-session wipe cannot lose it. Not saved: it is worth one latch to
re-earn, and a save version bump for a tutorial flag is not.

### 4. Two things that were never player text

*"HoldLine needs spline points before anyone can climb."*
*"This drive has no Travel Target set."*

These are levels placed wrong. The person who needs to read them is holding
the editor, and a four-second red flash during a playtest is the surface
most likely to be missed. They go to `LogDirtbagSetup` now — where they
outlive the playtest — and to a thirty-second toast that names the actor.

## What stayed, and why

Everything else. Refusals (*"Not enough for that."*, *"Not this week."*,
*"Finish the hand first."*) are news about a decision you just made and
failed; you pressed the key, it said no, there is nothing pending. Results
(*"Ate. -$8"*, *"Off at move 6 of 11"*, the send line) are outcomes. In-move
feedback (*"shake -12 pump"*, *"over-gripped"*) is 1.5 seconds long on
purpose and is about a thing that has already happened.

A rule worth keeping: **the fire's walk-away settlement still toasts**,
because walking away takes the table with it and the sentence would
otherwise be drawn for no frames at all. That is the shape of the whole
sort — not "toast bad", but *does this outlive the moment it describes*.

## The eighth checker

Writing this cost a build cycle that never happened, and it is worth
recording how close it came.

Both files wanted the same setup-log category, so both got:

```cpp
DEFINE_LOG_CATEGORY_STATIC(LogDirtbagSetup, Log, All);
```

Two files, one line each, obviously correct in isolation — and **UBT
unity-builds by default**, concatenating several `.cpp` files into one
translation unit, where the second is a redefinition. Every existing
checker passed. There is no compiler here. It would have surfaced as a
redefinition error on Evan's machine in a file neither author was looking
at.

Fixed the way UE intends: `DECLARE_LOG_CATEGORY_EXTERN` once in a header
both files already include, `DEFINE_LOG_CATEGORY` once in `DirtbagUE.cpp`.

And `tools/check-logcat.py`, wired into preflight as check eight: a category
may be defined in at most one `.cpp`, and a category that something logs to
must be defined somewhere. Both halves verified by reintroduction — the
duplicate fires the first, deleting the definition fires the second.

**One thing the checker deliberately does not flag.** The first version
reported any declared-but-undefined category and immediately caught two of
Epic's own template headers, which declare `LogTemplateCharacter` and
`LogCombatCharacter` and never log to them. Those link perfectly well: an
extern nobody references costs nothing. The rule is use-aware instead of
whitelisted, because **a checker with a whitelist is a checker people learn
to ignore.**

## At the desk

Nothing to place, nothing to re-wire. The prompt panel appears the moment
you walk into any trigger. The one thing worth checking on purpose: walk up
to a **roped** line at the gym while tired, and confirm you can see the
belayer line *and* the body advice at the same time. Before this you could
only ever see the second.

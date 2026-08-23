# The body you walked in with

**2026-08-23.** Found by accident, half an hour after Phase 10 shipped,
while answering the question *"so what can you do next?"*

## What it was

`AttemptProblem` — the comp resolver — built its own `AttemptInput` from
scratch and set **six fields**:

```
in.climber  in.route  in.oddsPenalty  in.padding  in.warmth  in.cleanliness
```

It never set the joints, the ailments, the comeback stage, the temperament
or the rubber. And it *assigned* `oddsPenalty` from the comp's pressure
dial, over the top of the climber's flaw.

Measured, 400 boards, the same climber twice:

```
comp score, healthy climber:      4.1
comp score, same climber wrecked: 4.1
```

The "wrecked" climber had a maxed-out finger joint, four cortisone shots, an
old scar, a bad flu, an abscess and dead rubber. Through the session path
the same body carried **2.2 grades** of ailment penalty and a fully degraded
joint. At a comp it carried none of it.

That path serves gym comps, the circuit, the World Cup, the Games **and**
league nights — most of the game's indoor time. Phase 7 (who your climber
is) and Phase 10 (the body keeps score) were both invisible there, and
Phase 3's shoe economy was silently free.

## Why nothing caught it

Twelve checkers, 78,000 assertions, and none of them could see it:

- `check-unwired.py` proves a sim declaration is **reachable**.
- `check-doors.py` proves an engine verb is **callable**.

Both are questions about *names*. This is a question about **a struct being
filled in**, and there was no checker that asked it.

It is the same shape as every "written and never wired" this project has
found — and the same shape as `Injury::staged`'s two owners from the day
before: **two paths, one of them assembling by hand.**

## The fix

`Sim/DirtbagBodyContext.{h,cpp}`. One struct for everything the body carries
into any attempt anywhere, and one function that stamps it:

```cpp
struct BodyContext { Character who; Medical medical; Sickness sickness;
                     Teeth teeth; double shoeWear; int day; };
void ApplyBody(AttemptInput& in, const BodyContext& body);
```

**Here: the body.** Who they are, what the joints carry, what they caught,
what their teeth are doing, what is left of the rubber. All of it follows
them everywhere and none of it cares where they are standing.

**Not here: the situation.** Warmth, mats, cleanliness, beta and the nerves
of a competition are exactly what make a comp different from a Tuesday, and
they belong to the caller. `ApplyBody` never touches them.

Additive where the caller has already had its say — **a flaw and a comp's
nerves are two reasons the odds are worse, not one silently replacing the
other.**

## The checker

`tools/check-bodycontext.py`, and preflight is thirteen now.

> **Any function that declares a local `AttemptInput` must call
> `ApplyBody` on it.** One escape, `// body-ok: <why>`.

A struct *field* of that type is not a build site — it holds one stamped
elsewhere — and is skipped, because a checker that flags every field is
crying wolf, which is the one thing a checker must never do.

Verified by putting the bug back: the checker names the file and line, and
five assertions fail.

## What it is worth

Thirty years, a comp-focused career, three ways of looking after yourself:

| | podiums | ranking at 30y | league best | injuries |
|---|---|---|---|---|
| no care | 93 | 267 | 169 | 11 |
| looks after it | 93 | **524** | 152 | **0** |
| impatient | 39 | **75** | 124 | 71 |

A **seven-fold spread on the ranking**, driven entirely by how a climber
looks after themselves. Before the fix, all three of those columns resolved
their comps identically — the only difference was days lost to being hurt.

## The lesson, stated plainly

Every checker in this project so far asks *does this name reach that name*.
That is the wrong question for the class of bug where a caller reaches the
callee and hands it half of what it needed.

**The hole was found by accident.** The next one of this shape will be too,
unless the question gets asked deliberately. Worth a sweep: every struct
that more than one path assembles by hand is a candidate.

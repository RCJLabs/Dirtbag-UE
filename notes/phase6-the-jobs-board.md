# The jobs board

*2026-08-23. Phase 6. Second of the five no-door systems.*

## What the door was hiding

`TodaysJobBoard` and `TakeOddJob` were both `BlueprintCallable` with no
caller. The Shift spot said **"Take a shift? (E)"** and called
`WorkShift()` — a flat 4 hours, $60, 30 energy, every day, forever.

Wiring the board turned up something much worse than a missing menu.

## The measured game and the played game were different games

`Sim/tools/season.cpp` — the probe behind **every economy measurement this
project has made** — works the board properly. It reads the three gigs,
skips the ones that need a van it does not have, and a careful policy even
declines the guidebook photo gig when its standing is already low.

The game the player was actually playing did none of that.

And the difference is not mostly the money. It is this, in `WorkOddJob`:

```cpp
if (job.name.find("guidebook") != ...) TookTheGuidebookPhotos(standing);
else if (job.name.find("trail work") != ...) DidTrailWork(standing);
else if (job.name.find("setting") != ...)    SetAtTheGym(standing);
```

`WorkShift` has **no standing effects at all.**

So in the played game, **work and reputation were completely
disconnected** — while in every measured career they were joined. You work
most days of a climbing life, which makes the board the faction system's
single most frequent input, and it was not plugged in.

Every number in `notes/phase3-jobs.md`, `notes/phase3-economy.md`, the
dreams measurements and the hoarder sweeps was measured on a game that
existed only in the harness.

## What the board actually offers

Ten gigs, three on the board each day, drawn without repeats, with pay
wobbling ±15% so the same gig is worth having some weeks and not others:

| | hours | pay | energy | van | standing |
|---|---|---|---|---|---|
| washing dishes at the diner | 5 | 60 | 20 | | |
| a shift at the gear shop | 4 | 55 | 12 | | |
| setting at the gym | 5 | 95 | 35 | | gym |
| moving somebody's furniture | 3 | 65 | 40 | ✓ | |
| hauling firewood | 4 | 70 | 45 | ✓ | |
| trail work for the park | 6 | 80 | 38 | ✓ | **stewards up** |
| flyering for the festival | 3 | 35 | 8 | | |
| **shooting photos for the guidebook** | **2** | **130** | **10** | ✓ | **old guard and stewards down** |
| stacking shelves, night shift | 6 | 85 | 30 | | |
| belaying kids' birthdays | 4 | 50 | 15 | | |

The flat shift was $15/hour. The board runs from $11.67 (flyering) to
**$65/hour** for the guidebook photos — which is five and a half times the
worst gig, takes two hours, costs almost no energy, and is the one that
turns the valley against you.

**That is the whole system in one row**, and no player has ever been
offered it.

## The door

The prompt *is* the board — no opening ceremony, because a board is a thing
on a wall that you read. Three numbered lines with pay, hours and energy,
because those are the three things you are choosing between. `(needs the
van)` appears only when the van is off the road; a gig needing a van you
have is not worth a word.

**1/2/3 takes one. E does not.**

E deliberately does not take the best gig, and that is the one design
decision here worth defending: the gigs are not interchangeable, the
best-paying one costs you the old guard and the stewards both, and a key
that silently picked would hand somebody a standing hit they never chose.
E says *"It's all on the board. 1, 2 or 3."*

When the van is dead, the refusal is stated as a reason rather than a
failure — *"hauling firewood needs the van, and the van is off the road"* —
because that is one more place the breakdown bites: it costs you the fix
**and** the work that would have paid for it.

## Two bugs fixed on the way

**The dog only existed on one path.** `WorkShift` applied van guilt — the
psyche cost of leaving a dog in a hot van while you work — and `TakeOddJob`
did not. Since the board was about to become the *only* way to work, wiring
it as-is would have quietly deleted the entire van-guilt system by making
its one caller unreachable. It moved.

**`UDirtbagGameInstance::WorkShift` is gone**, and the checker missed it.
`check-doors.py` counts `Name(` across the module, and
`UDirtbagSimLibrary::WorkShift` shares the name — so the game instance's
version looked called when its only "caller" was its own body calling the
other one. Same blind spot `check-cues.py` had with `Van`, for the same
reason: **a grep sees names, not scopes.** Found by hand, and now written
into the checker's docstring along with the cheap fix — do not name a verb
something the library also has.

## At the desk

1. **Walk to a work spot.** The prompt should be three gigs with prices,
   not "take a shift".
2. **Press E.** It should point at the board and do nothing else.
3. **Take the guidebook photos a few times**, then look at your standing on
   the HUD. It should start costing you. That connection has never existed
   in the played game.
4. **Break the van, then look at the board.** Van gigs should say so, and
   pressing their number should explain rather than fail silently.
5. **Adopt the dog, work on a hot day.** The van-guilt line should appear —
   it used to only appear for the shift that no longer exists.

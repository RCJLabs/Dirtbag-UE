# The day that never happened

*2026-08-24*

Phase 11 pass 2 ended with a number recorded and not explained: a measured
thirty-year career ate **595 meals**, one every eighteen days, and was
therefore a stranger in its own town — so every figure about the people
behind the counters was measured on somebody who was almost never in front
of them. The note called it a probe artefact and said fixing it would move
every measurement in the repo.

It is not a probe artefact, and fixing it moved more than the measurements.

## Hunger only counted the hours you spent

`PassHours` is the only thing that ever made you hungry, so hunger counted
the hours you *did something with* and nothing else. A day you found nothing
to do was a day you did not get hungry. Walk back to the van at ten in the
morning, sleep, and the night was free.

**The engine does exactly the same thing.** A player who presses sleep at
10am skips the day at the same discount, so this was never the probe
diverging from the game — it was both of them agreeing on something wrong.
Which is why the fix belongs in `SleepToNextDay`, the one function both
consumers pass through:

```cpp
// The rest of the day happened.
if (day.hour < dials.bedtimeHour) {
  PassHours(day, dials.bedtimeHour - day.hour, dials);
}
```

Bedtime is ten, flat, rather than last light — using the light would make a
winter cheaper to live through than a summer, and you go to bed when you go
to bed.

**The probe had the same bug in miniature, four times.** `today.hour = 21.0`
and friends set the clock instead of spending it, so a league night, a World
Cup round and the Games each cost their hours and no hunger at all. All four
now go through `PassHours`.

## ...and then the trap turned out to be unreachable

With the day passing properly, the obvious next question is whether anybody
ever actually goes to bed hungry. The answer was **no, and never had been.**

Hunger reset to nothing at `WakeUp`. Fifteen waking hours at three an hour
is forty-five. `starvingHunger` is seventy. **Seventy could not be reached
inside one day however the day went**, so the line that has been in
`DirtbagDay.h` since Phase 1 —

> *Sleeping hungry ruins the night's recovery — the dirtbag's oldest trap.*

— had never once fired, in any career, in any measurement this repo has ever
taken. The probe's own `starved` column has been printing `0` next to it the
whole time, which is the tell nobody read.

The missing half is that hunger has to survive the night:

```cpp
player.hungerCarried = day.hunger * Clamp01(dials.hungerKeptOvernight);
```

On the career rather than the day, because a `DayState` does not survive the
night — every caller builds tomorrow with `WakeUp`, which is exactly where a
carry stored on the day would have been silently thrown away. That is the
same "written and never wired" shape the checkers exist for, caught this time
by asking where the value would live.

At `hungerKeptOvernight = 0.6` the arithmetic gives the trap its intended
shape:

| night | wake | bed | |
|---|---|---|---|
| 1 | 0 | 45 | fine |
| 2 | 27 | **72** | first bad night |
| 3 | 43 | 88 | worse |
| ∞ | 67 | 112 | floor of the recovery |

**One missed day costs nothing and two is trouble**, which is what the dial
was always describing.

## What moved

Thirty years, three seeds, same policies.

| | before | after |
|---|---|---|
| meals | 595 | **8,148** |
| spent on food | $2,692 | **$35,674** |
| how well the town knows you | 0.45 | **1.00** |
| greeted by a send | 3–13 | **16–25** |
| greeted with *"thought you'd moved on"* | 35–37 | **0** |
| hungry nights | 0 | 0 (a solvent career can afford to eat) |

The town half is the headline: a career now eats three days in four, is a
regular everywhere, and **nearly every send gets said back to it** — which
is what Phase 11's third gate was supposed to feel like and previously only
technically passed. Nobody says *"thought you'd moved on"* any more, because
nobody has.

Climbing is unmoved — sends 22–26, all-round grade 4.1–5.5, injuries,
physio, quirks and the trad rack all land where they did. The food money
came out of slack the career was sitting on, which is the correct place for
it to come from and the reason the fix is affordable.

**Hungry nights are still zero**, and that is the right answer rather than a
remaining hole: these careers can afford to eat. The trap is live and tested;
it is waiting for a career that cannot, which is a broke career's problem and
now actually has teeth when it arrives.

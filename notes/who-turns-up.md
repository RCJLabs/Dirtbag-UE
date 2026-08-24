# Who turns up

*2026-08-24*

Three separate notes pointed at the same missing mechanic, and none of them
called it the same thing.

- **Phase 7** left `Personality::social` unwired, and said why in the
  header: *"`social` is whether people turn up, and in this game nobody ever
  fails to turn up. There is nothing for the axis to bend until turnout
  exists."* It shipped the axis stored, saved and asserted, with a note that
  wiring it *"the day turnout arrives is one line rather than archaeology."*
- **The Cave measurement** found `nobelayer` at **0 across three thirty-year
  careers** — the belayer gate, the thing that makes a rope crag different
  from a boulder, had never once bitten.
- **The roadmap's own standing question** is *"the Lot never varies, the
  same three neighbours across ninety years."*

One mechanic, three names. `LotRegulars` handed back all five people, every
day, for ninety years.

## The cast and the call sheet

`LotRegulars` is the cast — five authored people with their own careers,
who take their own lines whether or not you saw them. `WhoIsAround` is the
call sheet: who is actually here today.

Each of them has an authored `showsUp`, because who is around is a fact
about somebody's life rather than a distribution:

| | | |
|---|---|---|
| Trish | 0.82 | psyched beyond all reason |
| Ray | 0.88 | a neighbour; he is always about |
| Margo | 0.70 | twenty seasons here |
| Dev | 0.50 | young, strong, and busy |
| Bo | 0.45 | passing through, for the last four months |

Three things bend it, and each was already a system waiting for a reader:

- **Rapport**, +0.25 at most. Not *they come because you are there* — you
  know their week, so you turn up when they do.
- **`social`**, ±0.30, and **signed**: a Loner at −100 loses exactly what an
  Influencer at +100 gains. That is the difference between an axis and a
  perk, and it is the whole reason the axis exists.
- **The weather**, −0.45 on a day the rock never comes good. The loneliest
  day in this game is now the one where nothing was ever going to happen,
  which is also the day it should be.

Rolled per person per day on its own derived stream, so who is here does not
depend on the order anybody was asked about, and a reload hands back the
same Lot.

## The floor and the ceiling, which measurement demanded

The first build had neither, and the axis did not bend the mechanic — **it
switched it off**. Over thirty years of sport climbing, 8 seeds:

| | turned away |
|---|---|
| The Purist (social −25) | 89.6 days |
| The Influencer (social +55) | **0 days** |

Zero. Not rare — never. The effect is that violent because it applies to
three people *independently* and their absences multiply, so a 24-point
swing in each person's chance is an 8× swing in the chance that all three
are out.

`neverLessThan = 0.05` and `neverMoreThan = 0.95` put both ends back inside
the game: the most gregarious climber in the valley still gets the odd empty
Tuesday, and the most solitary one still has Trish turn up. After:

| | turned away |
|---|---|
| The Purist (social −25) | 89.2 days |
| The Influencer (social +55) | **4.5 days** |

Still decisive — the Influencer is turned away less often in **8 of 8
seeds** — and no longer immune.

## What it closed

**The belayer gate bites.** A thirty-year sport career now finds nobody to
tie in with on **27.3 days**, against 0 before, and 14.3 days end early
because a belayer had had enough rather than because your skin had. A
boulderer still measures 0, which is correct: they never needed one.

**`social` has a reader**, and the assertion Phase 7 left behind — written
so the axis would already be here the day turnout arrived — now measures the
thing rather than standing in for it.

**And rapport is honest.** `SpendDayWith` was called for the whole cast on
any day you climbed, so a day you climbed was a day you climbed *with
everybody*. It now moves only for people who were actually there, which is
a fix the turnout mechanic made possible and also made necessary.

## What it did not close

Rapport still ends at **0.45 with everybody**, the `everKnew` floor, whether
a career climbed 99 days or 2,085. Turnout does not touch that — it is
`rapportPerDay` saturating in thirteen days, and moving it is four systems
wide and a design call about *where you get to know people, at the fire or
on the rock*. Still Evan's.

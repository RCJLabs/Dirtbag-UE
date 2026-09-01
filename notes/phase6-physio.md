# Physio gets a door

*2026-08-23. Phase 6. Fourth of the five no-door systems.*

## What was missing

`SeeAPhysio` was complete, tested, and callable by nothing.
`concepts/DIRTBAG.md` §4 ports **injury + physio + aging** as one thing:
injury is built and measured (`notes/phase4-getting-hurt.md`), aging is
built, and **the recovery half had no way in**. You got hurt and you waited.

That is the half that matters most, because the sim's own dial comment says
what it is for:

> *"The one thing in the game money buys that gives you climbing back rather
> than moving it around."*

Every other purchase in this game moves climbing around — a shift is hours
you were not on rock, kit is volume you buy with hours you already worked.
Physio is the only one that hands days back.

## The door, and where it is not

**`P` at the gear-shop counter.** Its own key, for the same reason signing
has one: E buys rubber.

The counter is not a physio and I know it. There is no physio *place* — the
town is not somewhere yet — and the shop is already the town's one desk,
handling rubber, pads, dreams and a sponsor's paperwork. **When the town
becomes somewhere in its own right this earns its own door**, and that is
written where whoever builds it will read it.

The prompt states the trade in the only units that matter: *"See a physio?
(P) — $95, and about 6 days off it."* Days, not "recovery". And when it
refuses it says which of the three reasons it is — too soon, too poor, or
you are not hurt — as a physio saying so rather than as a locked door:
*"The physio wants 4 more days before they see you again."*

## Measured: what does it buy?

| injury | untreated | treated | cost | $/day bought back |
|---|---|---|---|---|
| 7 days | 7 | 1 | 95 | 15.80 |
| 14 days | 14 | 8 | 190 | 31.70 |
| 30 days | 30 | 15 | 285 | 19.00 |
| 60 days | 60 | 30 | 475 | 15.80 |
| 120 days | 120 | 64 | 950 | 17.00 |

**Physio roughly halves an injury**, at a stable ~$16–19 a day across every
length. (The 14-day row's $31.70 is a rounding artefact of the seven-day
cadence, not a different price.)

Strong, and meant to be — this is the answer to a gate that has been open
since Phase 3, where money has never once bought climbing.

## The finding that matters, and it is not the price

**The measured career cannot afford it.**

A full course for a thirty-day injury is **$285**. And
`notes/what-the-2d-game-still-has.md` measured a thirty-year `saver` career
— the policy that actively tries to accumulate — and found:

> **highest cash ever held: $447**

Across ninety years and four generations: $640,665 earned, **cash high
$417**. `notes/phase3-kit.md` measures a year's cash high at around $240.

So a full course of physio is **64% of the most money that career ever held
at once**, and more than a whole year's peak. It is affordable roughly once,
at the top of the best year a dirtbag ever has.

That is not necessarily wrong — it is a *dirtbag* game, and "I cannot afford
the physio" is the most authentic sentence in it. But it means the system's
strength is theoretical for most careers, and nobody could have known that
while the door did not exist.

**Two things have changed since $447 was measured**, and both cut the other
way: dreams gave careers a reason to bank, so balances are held rather than
spent; and the sponsor stipend — $120 to $640 a month — became reachable
about an hour ago and is the first income in this game that does not cost
climbing time.

**Worth re-measuring at career scale** with a probe policy that sees a
physio when it can. That is the honest next step and it is a measurement
rather than a guess: does a sponsored climber with a dream ever actually
buy their season back?

## A smaller mismatch, recorded

The dial comment says physio is *"priced against a week of bills ($85),
because that is the trade — you are buying a fortnight of your season
back."* One session is $95 and buys six days. A fortnight back costs
**$285**, three sessions across three weeks. The framing is per-session and
the claim is per-injury; the number is right and the sentence is loose.

## At the desk

1. **Get hurt.** The gear-shop prompt gains a line saying what a physio
   costs and what it buys.
2. **Press P.** *"An hour of somebody digging their thumb into it."* The
   injury line should jump forward about six days.
3. **Press P again the next day.** It should tell you the physio wants six
   more days, as a sentence rather than a refusal.
4. **Try it broke.** It should say the price and what you have, and do
   nothing.
5. **Notice whether you can ever actually afford it.** That is the open
   question above, and you will hit the answer before any probe does.

# Shoes: the cheapest thing that matters, and it did nothing

**2026-08-21.** `ShoePenalty` came off the reachability sweep flagged as
unwired, and it turned out to be unwired for an unusual reason: **the
resolver had its own copy of the formula.**

```cpp
// DirtbagGear.cpp                       // DirtbagSession.cpp, inline
gone = Clamp01(shoes.wear);              rubberGone = Clamp01(input.shoeWear);
deadShoeGradePenalty * gone * gone       deadShoeGradePenalty * rubberGone
  * (edging ? 1 : biteOnGoodHolds)         * rubberGone * (edging ? 1 : ...)
```

Identical, and reading from **two different dial structs** — `GearDials` and
`SessionDials`, whose comment already admitted the arrangement: *"Mirrors
GearDials so the resolver and the shop agree; the numbers live there."*

That mirroring is deliberate and fine. It is what keeps `DirtbagSession` from
including half the project to price a move, and two other values do the same
thing (the runout mirrors `SportDials`, the injury penalty mirrors
`BodyDials`). The rule it encodes is **share the function, mirror the
constant**.

Shoes were doing neither. The numbers were mirrored *and* the formula was
written out again beside them, which is how `ShoePenalty` ended up with no
caller and how the duplication got noticed at all.

Now: `ShoePenaltyFor(wear, edging, deadPenalty, biteOnGoodHolds)` is the one
place dead rubber is priced. It takes the two numbers rather than a dial
struct, so the resolver can call it without the include. `ShoePenalty` — the
shop's way in — delegates to it. **The golden vectors did not move**, which
is what says the two copies really were identical rather than nearly so.

## And a guard on the other two, because a comment is not one

`TestTheMirroredDialsStillAgree` pins all three pairs:

| SessionDials | mirrors | value |
|---|---|---|
| `deadShoeGradePenalty` / `shoeBiteOnGoodHolds` | `GearDials` | 1.1 / 0.3 |
| `runoutGradePenalty` | `SportDials` | 1.1 |
| `injuryGradePenalty` | `BodyDials` | 2.6 |

Nothing would have failed if one of them moved. The shop would quote a price
for dead rubber the wall did not charge; the guidebook would describe a
runout the resolver did not price; the physio would disagree with the
climbing about what an injury costs. All silently, and all only visible as
"the numbers feel off".

## What the thing nobody has ever felt is worth

Worth measuring before calling it fixed, because until the `ToSim` fix
earlier today `shoeWear` arrived at every attempt as 0.0 — **every attempt
in the game has always resolved on brand-new shoes.**

A year, greedy, ten seeds, `norubber` never resoles or replaces:

| | sends | burns | allround | end wear | cash |
|---|---|---|---|---|---|
| buys rubber | **3.7** (0–7) | 520 | 5.50 | 0.29 | $170 |
| never replaces | **1.0** (0–6) | 517 | 5.44 | 0.99 | $161 |

Per seed, buying rubber **wins 7, ties 3, loses 0**. Same burns, same days —
the difference is entirely what those burns were worth.

And it costs almost nothing: **$9 of the season's cash**, against a shoe
spend the kit note measured at $275 out of $6,601 earned.

That puts shoes in the same category the kit measurement identified as the
only one that can add to a season: **kit that raises the value of skin you
were already going to spend**, rather than kit that spends more of it. The
crash pad was the other. It is the cheapest thing in the game that matters,
and until today it did nothing at all.

> Caveat kept in view: `sends` is a noisy metric at this sample size — the
> ranges overlap and one seed ties at 6–6. The per-seed pairing is what
> carries the claim, not the means.

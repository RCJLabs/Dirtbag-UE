# Dreams

**2026-08-22.** Third of the four unported systems, and the only one where
the measurement came before the design rather than after.

`concepts/DIRTBAG.md` names *"dreams (Rig/War Chest/Home Base)"* in the
port-wholesale list and never defines them. So this is **a design, logged as
one**, not a port.

## Why it is not priced in money alone

`notes/dreams-what-money-is-worth.md` killed the obvious version. Money is
not scarce: a career that banks hard climbs **970 days against 422** and
ends with sixty thousand dollars, because a float stops you bodging the van
and bodging is what costs you your season. **Saving is the best climbing
decision in the game.**

So a dream priced only in money is a timer. Evan's call, taken 2026-08-22:
**a dream costs the buffer.** You spend the float that was keeping the van
alive, and the reward arrives with a lean, fragile year attached.

## Three dreams, three currencies

Deliberately not a ladder — you should not be able to rank them.

| | price | what it buys |
|---|---|---|
| **the Rig** | $9,000 | **van uptime.** Everything new, and `VanDials::rigLifeMultiplier` 2.2× on part life. |
| **the War Chest** | $14,000 | **time.** 365 days where you never have to take a gig. |
| **Home Base** | $30,000 | **the body.** +0.5 skin a night for sleeping indoors, and $22 a day forever. |

The Rig is deliberately short of solved. The measured gap between a bodging
career and a floated one is 548 climbing days; a dream that handed all of
that over would end the van as a system rather than upgrade it. **It does
not stop breaking, it stops breaking so often.**

Home Base is the only one carrying an ongoing cost, because an address is
the one that does not stop asking. Rent behaves like every other bill —
cash floors at nothing and the shortfall waits. **An address you cannot
afford is a debt with a door on it, not a repossession.**

`Dreams::working` is what you have *said* you are saving for. Free to
declare, free to change, and it does nothing on its own. **Declaring is not
the commitment; buying is.**

## It bites

Thirty years, three seeds, `hoarder` (banks and never spends) against
`dreamer` (buys each the day it can afford it):

| | hoarder | dreamer |
|---|---|---|
| **days worked** | ~2,570 | **~5,200** |
| days stranded | 113 | **51** |
| days climbed, mean | 653 | 519 |

**Working days double, across every seed.** That is the buffer being spent
and rebuilt, three times over a career — the cost is real and it is paid in
the only currency the game has ever charged in, which is days.

And the Rig delivers what it promised: **stranding halves.**

The climbing figure is the honest one to be careful with. 653 against 519 is
a mean over three seeds and the spread is wide (crag-1: 965 against 457;
crag-3: 616 against 644 — the dreamer *ahead*). What the numbers support is
that a dream costs roughly a fifth of a career's climbing days and buys
three things you wanted. What they do not support is a precise price on
that, and three seeds is not enough for one.

## Where it surfaces

- `DreamCost` / `DreamName` / `DreamBlurb` / `CanAffordDream` — the shop.
- `WorkTowards(Which)` — say what it is for. Costs nothing.
- `BuyDream(Which)` — takes the cash and hands back the thing. `DreamNews`
  says it plainly the day it happens: *"the Rig. And 40 dollars left to your
  name."* The game should not be coy about what it just did to your float.
- `DreamLine()` — with the slow numbers.
- `FreeDayLine()` — *"Nothing needs doing today."* Drawn in the same eyeline
  as the debt and the van, because it is the same question answered the
  other way: what does today need from you. **Only the War Chest ever
  answers "nothing"** — being flush is not the same as not having to think
  about it.

## Save

`kSaveVersion` 17 → 18, `MigrateV17ToV18`. An old career owns nothing and is
saving for nothing, which is exactly right — there was nothing to own.
`van.rig` is false for the same reason: **a Rig is a van you bought, not a
van you maintained**, however much has been spent keeping the old one alive.
`dreams.working` is clamped on load rather than trusted, the way the injury
kind is.

## Still open

Two things, neither blocking:

1. **Is any of this what the 2D game means?** Three words was all the repo
   had. The prices and effects are dials; the *shape* — three dreams, three
   currencies, cost is the buffer — is the part worth arguing about if the
   2D version disagrees.
2. **Nothing asks the player to choose one.** They are three purchases in a
   shop, made in whatever order the money allows. The version where naming a
   dream early closes off the others was the third option on the fork and is
   still available on top of this one.

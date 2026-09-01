# Sponsorship gets a door

*2026-08-23. Phase 6. Third of the five no-door systems.*

## The money that cost nothing

`SignWithSponsor` had no caller, so there was no way to have a deal at all.
That much was expected.

**`SponsorOwnsToday` had no caller either**, and that is the whole system.
The sim header is explicit:

> *"A shift lands on any day. A photo shoot lands on a good one. […]
> Obligation days a month, and they land on days with a window. **This is
> the entire mechanic**: money that costs you the good days rather than the
> spare ones."*

Nothing asked it. So even if a deal had been reachable, it would have paid
$640 a month and cost **nothing** — the one money in this game whose price
is good days would have been the only free money in it.

## Both doors

**Signing is at the gear-shop counter, on `S`.** Its own key because E buys
shoes, and signing away three days a month should not be the same press as
buying rubber. The counter only mentions it when the offer beats what you
already hold — a shop that offers last year's deal every visit is a shop
nobody reads.

The offer always states **both halves**: what it pays *and* what it will
want. *"a title sponsor - $640 a month, and 3 days a month that will not be
the rainy ones."* An offer that says what it pays and not what it wants is
an advert, and the price here is days.

**The obligation is taken at Sleep**, not offered as a choice. You wake up
and six hours already belong to somebody, which is what being sponsored is —
and the decision was made when you signed. The sim's header calls the lost
day *the entire mechanic*, so making it dodgeable would be building a
different system.

The news is appended rather than assigned: the month's money, the yearly
review and a shoot can all land on the same morning, and overwriting would
lose whichever mattered.

## Measured: what is a deal worth?

Five years, eight seeds, counting the days actually taken rather than the
dial's ceiling — `ObligationToday` only fires on a day with a window, so the
weather decides the rest.

| tier | $/year | days taken | good days | $ per good day |
|---|---|---|---|---|
| shoes | 0 | 0 | 0 | — |
| gear | 1,440 | 6.0 | 6.0 | 242 |
| title | 7,680 | 18.0 | 18.0 | 426 |

**Every day they take is a good day.** The mechanic works exactly as
written — the dial is a ceiling and the window is the filter, and there is
no leakage onto rainy days.

For scale: a thirty-year career earns about **$7,461 a year** from working
(`notes/dreams-what-money-is-worth.md`). So **a title deal roughly doubles
your income and takes about 15% of your good days.** That is a real trade
and a temptingly good one, which is the right shape — and it only became a
trade at all once dreams gave money somewhere to go.

The shoe deal is pure upside: nothing paid, nothing owed, and $165 a pair
saved. Correct — it is the rung that says somebody noticed, and a first
season's reward should not have a hook in it.

## Two things found and not fixed

**The obligation counters are dead in three layers.**
`obligationsMetThisSeason` and `obligationsMissedThisSeason` are: not
mirrored to the engine, not saved, and — the part that matters —
**`ReviewSeason` resets them and never reads them.**

The mirror-skip comment says they are *"within-season bookkeeping the review
reads and then clears."* It does not read them. The comment is wrong about
the only thing it asserts, which is how the dead field survived.

They are vestigial from a design where missing a shoot could cost you the
deal. That design is not the one the header describes, so **I have not built
it** — making missing possible and costly means mirroring two fields, a save
version bump, a new `ReviewSeason` arm and its tests, and it would change a
measured system on my own initiative rather than Evan's. Recorded instead,
with what it would take.

## And the fifth ethical act is unblocked

`StagedAPhoto` could not be offered yesterday because its benefit was a
sponsor's goodwill and there was no way to get a sponsor. There is now.

It buys exactly one thing: **the review's clock goes back to zero.** You did
not send anything and they think you did. Deliberately *not* a fake tick in
the ledger — the lie is told to the sponsor rather than to the book, which
is why its `Secret` carries no route key and why `StripsTheAscent` finds
nothing to take when it comes out. The ascent never existed to be taken.

Offered only when you have a sponsor *and* a season without progress behind
you — a photo of a send that did not happen is a thing you do the season
before a review goes badly, not a thing you do for fun.

**And that exposed a bug in yesterday's work.** The wall had three shortcut
keys and a comment claiming all four acts could never be offerable at once.
They can, and it is the most interesting case in the system: a **sponsored
climber in a slump, standing under a line they have already fallen off** can
chisel it, tick it, pull through on it, or shoot it. That player would have
been shown a fourth option with no key to press. Four keys now, and the key
indexes the *offered* list rather than the master list, so 2 always means
the second thing the prompt actually showed you.

## At the desk

1. **Climb something hard, get some first ascents, keep the Scene sweet.**
   The gear-shop prompt gains a line when somebody starts asking.
2. **Press S.** It should say what it pays and what it wants, in one line.
3. **Then sleep, repeatedly.** On days they own you wake up around 15:00
   with the shoot done and the window gone. That is the deal you signed.
4. **Have a flat season while sponsored, then stand under a line.** T should
   offer to shoot it like you got it.
5. **Watch the yearly review.** Two flat seasons drop you a rung, unless you
   were hurt, or unless you staged something.

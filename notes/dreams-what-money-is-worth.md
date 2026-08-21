# Before dreams: what is money actually worth?

**2026-08-21.** Evan: *"Let's work on dreams."*

A dream is a thing you save for. Before pricing one, I wanted the number
nobody had: **what can a career bank, and what does banking cost in
climbing?** The answer inverted the assumption I was about to design against.

## Nobody had ever saved

`saver` sounds like the policy for this and is not — it buys pads. Its own
comment says the pad is *"the only thing in the game worth saving for"*,
which is precisely the hole dreams exist to fill. **No probe policy has ever
banked money.** Same shape as the crash pad, the projector and the stakeout:
the fourth time this project has found a mechanic nobody had tried.

So: `hoarder`, which is `stakeout` plus working towards a number instead of a
thermostat. Thirty years, crag-1, sweeping the target.

| target | peak cash | worked | **climbed** | burns | FAs |
|---|---|---|---|---|---|
| $0 (thermostat) | $417 | 2,202 | **422** | 1,202 | 2 |
| $10,000 | $10,148 | 2,394 | **425** | 1,193 | 2 |
| $25,000 | $25,147 | 2,544 | **962** | 2,898 | 3 |
| $60,000 | $60,148 | 2,893 | **970** | 2,920 | 3 |

**Saving does not cost climbing. It buys climbing.** The hoarder works *more*
days and climbs *more than twice as many*, and banks sixty thousand dollars
doing it. There is no trade-off anywhere on this curve.

## The mechanism is the van, and it was designed on purpose

Same seed, same weather — **5,056 days never came good in both runs.** The
difference is entirely in what could be done with the days that did.

| | broke | banked |
|---|---|---|
| breakdowns | **206** | 113 |
| **bodged** | **100** | **0** |
| hours under the van | **632** | 282 |
| days stranded | 206 | 113 |
| spent on the van | $8,700 | $18,355 |

`DirtbagVan.h` says it plainly: *"bodge — free, costs hours, and does not
hold for long."* Four hours and `bodgeRestores = 0.25`. A patch or a
replacement costs money and holds.

So a broke climber bodges a hundred times, and pays for it with **twice the
breakdowns, 350 extra hours under the van, and 93 more days stranded at the
side of the road**. A climber with a float fixes it properly the first time
and never bodges once. Over thirty years that is **548 climbing days** — a
year and a half of climbing, bought with money.

This is a poverty trap and a good one. It was built deliberately, it works,
and nobody had ever seen it because no policy ever had money.

## Which breaks the obvious dream design

The design I was going to bring: dreams cost money, money costs climbing
days, choosing a dream is choosing what to give up. **That tension does not
exist.** Money is not scarce and saving is not a sacrifice — it is the
highest-return climbing decision in the game.

A dream priced only in money would therefore be **a timer, not a decision**.
The player would reach it by playing well, at no cost, on a schedule.

That is worth saying plainly because it is also the answer to why the money
loop has felt thin through three phases: not "money has nowhere to go" as I
wrote in the audit, but something sharper. **Money already has somewhere to
go — the van — and it is the best purchase in the game. Nothing tells the
player, and nothing asks them to choose.**

## Three ways a dream could actually bite

1. **A dream costs the buffer.** Buying the Rig spends the float that was
   keeping the van alive, so the reward arrives with a lean, fragile year
   attached. The cost is real, it is temporary, and it is exactly the shape
   of a real climber buying a van they cannot quite afford. **This is the one
   I would build** — it needs no new mechanic, it uses the trap that already
   exists, and it makes the decision *when*, not *whether*.
2. **A dream costs days, not dollars.** Home Base means an address, which
   means rent and a job with hours. The dream that ends van life ends the
   dirtbag year with it. Thematically the strongest and the most work.
3. **A dream is a commitment made early.** You declare one, cheaply, and it
   changes what the following years are for — closing off the others.
   Cleanest as a *game* and the furthest from anything the repo currently
   models.

## What I have not done

No dream is built. `hoarder` is a probe policy and touches no game code.

The measurement is the deliverable, and it says the obvious version of this
system would not work — which is worth more than a system that quietly does
not bite.

# Ethics gets a door

*2026-08-22. Phase 6. First of the five no-door systems from
`notes/no-doors.md`.*

## What was missing was only the act

The discovery half was **complete and wired**: `SomebodyFindsOut` runs at
Sleep, `ItComesOut` applies the standing and psyche cost, `StripsTheAscent`
clears the send, and `EthicsText` reaches the HUD as `EthicsNews`. All of it
tested, saved, migrated.

`DoSomethingYouWouldNotAdmitTo` — the verb that starts the whole arc — was
callable by nothing. The consequences of an act you could not perform.

## The door

**T at the wall**, and the offer is a *prompt state* rather than a screen —
this is a thing you do in a moment at the bottom of a route, not a menu you
open. 1/2/3 takes one, T again thinks better of it, walking away withdraws
it.

The prompt says **"Nobody is watching."** and lists what this line will
take. It is offered without being urged: a bare `(T)` on the end of the
route line, never a sentence explaining what it would buy. The game does not
sell you this.

Nothing is congratulated. The act gets one quiet grey line and no comment —
the comment arrives years later, from everybody else.

## Three acts, and the benefit is the exact mirror of the punishment

`DoSomethingYouWouldNotAdmitTo` records only that it happened; its own
comment says *"the caller applies whatever the act buys, because the benefit
differs per act and belongs where it is felt."* So the benefits are the new
work, and each is built to be precisely what discovery takes back.

**Claimed a send** and **pulled on the gear** both write `bSent` into the
ledger. `StripsTheAscent` is true for both, and the discovery path already
clears exactly that field. Perfect symmetry, no new machinery.

Pulling on gear needs you to have *been* on it — one hang nobody saw is not
the same lie as ticking a line you never tied into, which is why it costs
0.3 against claiming's 0.7.

**Chipping a hold** was the interesting one. Its benefit is that the line
gets easier *permanently, for everyone* — and `StripsTheAscent` is false for
it, because it changed the rock and the ascent stands, hollow but real.

The obvious implementation is wrong: **the crag is regenerated from the seed
on every load**, so a grade written into a crag line evaporates at the next
save. The secret is what persists, so the rock is derived from the secret —
one clause in `GetRouteAt`, which is the single funnel every caller goes
through.

## Two acts deliberately not offered

**Retro-bolting** buys less runout, and runout is computed in `RunoutAt`
rather than carried on the route, so its benefit needs plumbing that does
not exist. A real act with a real cost dial waiting for it.

**Staging a photo** buys a sponsor's goodwill, and **there is no way to get
a sponsor** — `SignWithSponsor` is still on the no-door list. Offering a
shortcut whose benefit is a system you cannot reach would be this project's
favourite bug wearing a new hat.

Both are recorded in the code where somebody deciding to add them will read
it.

## Measured: does lying pay?

The door creates a balance question the system did not have before, so it
got measured. Thirty years, 24 seeds, secrets spread across the first ten
years the way a career accumulates them:

| claims | visibility | caught | ticks kept | standing lost |
|---|---|---|---|---|
| 10 | nobody | 4.8 | 6 | 1.19 |
| 10 | star | 9.6 | 1 | 1.53 |
| 40 | nobody | 17.9 | 23 | 1.85 |
| 40 | star | 38.7 | 2 | 1.86 |

**The design works and the door does not break it.** A nobody who claims
forty lines keeps twenty-three; a star who claims forty keeps two. Success
is what exposes you, exactly as the header promises — and it is
self-correcting, because the ticks you keep raise your grade record, which
raises your visibility, which is what catches you.

Two things worth writing down rather than fixing:

**Standing loss saturates at about 1.86.** Twenty claims and forty claims
cost the same reputation, because `Shift` clamps. So at scale the deterrent
is not the shame, it is the **stripping** — and that scales linearly and
forever. Defensible: once you are a known cheat, being a slightly bigger
cheat costs nothing socially, and that is true of the world as well as the
model.

**The header slightly oversells the quiet career.** It says a complete
unknown has *"rather better than even odds of it never coming out at all"*
across thirty years; measured, a single secret at zero visibility survives
**40%** of the time, not better than half. Same ballpark, and the claim was
made about a dial that has not changed — but the number is 0.4 and the
sentence says 0.5.

## At the desk

1. **Walk up to a line you have not done, outdoors.** The prompt should end
   in `(T)`.
2. **Press T.** *"Nobody is watching."* and the options. Chisel is always
   there; pull-on only appears once you have actually fallen on it.
3. **Take one.** One grey line, no fanfare. The HUD's slow numbers gain
   *"1 thing nobody knows"* — silent at zero, because a counter reading
   "0 secrets" every day of a clean career is a counter suggesting you get
   some.
4. **Chisel something, then climb it.** It should go easier. Save, quit,
   reload, climb it again — **it should still go easier**, which is the
   thing that would have broken if the grade had been written onto the crag.
5. **Then play.** Years later, you will wake up and everybody will know.

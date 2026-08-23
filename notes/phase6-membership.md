# Membership gets a door — and the gym was free

*2026-08-23. Phase 6. Fifth of the five no-door systems, and the one that
turned out to be a hole rather than a gap.*

## The gym was free

`RenewGymMembership` had no caller, which is what put it on the list. The
real finding is underneath that.

`dirtbag::GoToTheGym` checks `IsGymMember` — and **nothing called
`GoToTheGym` either.** The path a player actually takes to the gym is a
travel spot and then a wall, and **neither asked for a membership.**

So the whole system was decorative:

- `RenewMembership` had no door, so you could never buy one
- `KitDay` ticked a counter down that nothing read
- the save carried `kit.membership` faithfully across versions, and it
  changed nothing
- and the gym let you climb, indoors, in any weather, forever, for **$0**

`Sim/DirtbagKit.h` calls the membership **"the load-bearing one"**:

> *"A season has 157 days that never come good and 75 more spent resting
> skin, and every one of them is currently dead time no amount of money can
> touch. A membership converts them, and it is the first recurring cost in
> the game."*

The first recurring cost in the game was not charged.

## The measured game, again

`notes/phase3-kit.md` measured a probe **buying** memberships — badly, as it
happens, one of its own findings was *"the probe bought a membership it
could not use: renewing whenever it lapsed bought 126 days and used 27"*.

So the measured economy paid $75 a month for the gym and the played one got
it for nothing. **That is the second time in two days** that the measured
game and the played game have turned out to be different games — the jobs
board was the first, yesterday.

Both had the same cause: a sim function with the rule in it, and an engine
path around the side.

## The doors

**`M` at the gear-shop counter** renews it. The counter always says where
you stand, because being a member is a standing fact rather than a symptom:
*"Gym membership: 12 days left. (M) tops it up, $75."* or *"Not a member.
(M) — $75 for 30 days of weather nobody can take off you."*

**The wall is the gate.** Indoors, without a membership, E refuses: *"The
desk wants to see a membership."* And the approach prompt says so before you
press anything, so nobody drives across town to find out.

Refused at the wall rather than at the travel spot on purpose — you can go
to the gym, look at the board, and leave. It is the climbing that is sold.

**The hangboard came with it**, being the same header's third item and the
broke answer to the same problem: `H` buys one at the counter, `H` uses it
at the van, where it lives over the side door. One key, two spots, and where
you are standing says which you meant. Its two refusals are said as
themselves — *"You have already hung today."* and *"Not on this skin. That
is how you take a week off."*

## The checker's blind spot, fixed rather than documented again

`check-doors.py` had missed `GoToTheGym`, `RenewGymMembership` and
`WorkShift` — every one a verb whose only apparent "caller" was **its own
body forwarding to the sim function of the same name**. I documented that
limitation twice and it bit a third time, so it is fixed: a call qualified
by some *other* scope (`dirtbag::Foo(`, `UDirtbagSimLibrary::Foo(`) is a
different function that happens to share a name, and no longer counts.

**Fixing it immediately surfaced eight more**, all previously invisible:

| verb | what it is |
|---|---|
| `GoToTheGym`, `IsGymMember` | the gym — done here |
| `BuyHangboard`, `HangboardSession` | the hangboard — done here |
| `TakeSalariedJob`, `QuitSalariedJob` | **the salaried job** |
| `IsWorkable`, `StandingWith` | accessors, annotated |

A grep that sees names and not scopes will always have edges, but this one
had a shape — the sim-forwarding wrapper — and that shape is now handled.

## What is left, and it is the big one

**You cannot take the salaried job.**

`Sim/DirtbagJobs.h` is unambiguous about what that system is for:

> *"The trap is not that the salary is a bad deal. It is that it is a good
> one, and taking it is entirely reasonable, and a season later you are
> solvent and climbing V4."*

That trap is the point of the entire work system, it is measured in
`notes/phase3-jobs.md`, it is what a **Dirtbag Year** is defined against —
365 days without signing for the nine-to-five — and **it has never been
possible to sign for.** Which means every Dirtbag Year ever counted was
counted against a temptation that could not be accepted.

Recorded as `no-door` rather than built, because it is a bigger design
surface than a counter key: it owns your week, breaks the streak, and wants
a quitting cost. Two verbs, and worth deciding deliberately.

`GoToTheGym` on the game instance is **deleted** rather than annotated — it
was a redundant second path to a place the player already reaches, and now
that the reachable path is gated, a second copy of the rule is exactly how
the two would drift apart. The sim's version stays for the probe, marked.

## At the desk

1. **Drive to the gym without a membership.** The wall should say the desk
   wants to see one, before you press E and again if you do.
2. **Buy one at the shop with `M`.** Then climb indoors.
3. **Sleep thirty times.** It should lapse, and the counter should say so.
4. **Buy a hangboard with `H`**, then press `H` at the van on a rest day.
5. **Notice the gym now costs $900 a year.** That is a real change to an
   economy whose measured cash high is $447, and it is the number the next
   career-scale measurement should be aimed at.

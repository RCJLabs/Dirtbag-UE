# What else is written and never wired

*2026-08-22. Evan asked. I hunted rather than guessed, and the answer is
five systems.*

## The bug class, six instances deep

| found | shape |
|---|---|
| the fire had no verb | a system with no way to play it |
| dreams had no counter | money with no destination you could pick |
| `Dreams::working` | a field written and never read |
| the handover had no door | **a whole phase**, unreachable |
| the guidebook had no page | tracking with nothing to see it in |
| `PeakGradeEver` | **a live function answering from constants** |

Every one was found by hand, late, and by accident. So this time it gets a
checker.

## The five systems with no door

Hunted by taking every `BlueprintCallable`/`BlueprintPure` on
`UDirtbagGameInstance` and counting real call sites — telling a definition
from a call by whether a `{` follows the parameter list, which is the only
way that works without a parser. Then confirmed by hand: **none of these is
reachable from any spot, wall, or HUD.**

### 1. The odd-jobs board — `TodaysJobBoard`, `TakeOddJob`

`concepts/DIRTBAG.md` §4 lists it as **port-wholesale**. The Shift spot is a
generic *"Take a shift? (E)"* — it never touches the board. So the whole
system of choosing *which* work to take, which is the interesting half, is
unreachable.

### 2. Physio — `SeeAPhysio`

§4 lists **injury + physio + aging** as one port. Injury is built and
measured (`notes/phase4-getting-hurt.md`); aging is built. **Physio — the
recovery half — cannot be visited.** You get hurt and you wait.

### 3. Gym membership — `RenewGymMembership`

Nothing renews it, and membership gates the board and the kit. Worth
checking at the desk what `IsGymMember` currently returns on a fresh save,
because if it is false the gym may be unusable and if it is permanently true
the fee never bites.

### 4. Sponsorship — `SignWithSponsor`, `SponsorOwnsToday`

Phase 4 scope, built, measured, saved, migrated. The HUD prints
`SponsorLine()` and `SponsorNews`, so a sponsorship you somehow had would
display — **but there is no way to sign one**, and nothing ever asks
`SponsorOwnsToday`, which is the obligation half: the days the deal owns.

### 5. Ethics — `DoSomethingYouWouldNotAdmitTo`, `ThingsNobodyKnows`

§4 port-wholesale. `EthicsNews` reaches the HUD, so consequences would
show — **but the verb that causes them cannot be pressed**, and the readout
of what you are carrying is never drawn.

## Not gaps, and why

Seven more came back and are legitimate:

- `NameFirstAscent`, `DismissNaming` — reached from `WBP_NameFirstAscent`,
  the one Blueprint this project has. **Worth verifying at the desk**: the
  only `CreateWidget` in the whole module is Epic's mobile-controls one, so
  if nothing in Blueprint creates that widget, naming a first ascent is
  unreachable too. The HUD's *"FIRST ASCENT — X is yours to name"* line was
  deliberately added as a backstop for exactly this, so the moment is never
  invisible — but the backstop is not a door.
- `GetBoard`, `GetBoardRoute`, `GetCrag`, `NumRoutesHere`,
  `GetFirstAscents` — Blueprint accessors. C++ reads the members directly.

## The tenth checker

`tools/check-doors.py`. Every Blueprint-exposed verb on the game instance
must have a C++ call site, or say why not:

    // blueprint-only: <why>   reached from a widget rather than from C++
    // no-door: <system>       a real gap, recorded on purpose

**Scope is deliberately just the game instance.** `DirtbagSimLibrary` exists
to expose the sim to Blueprint, so an unused entry there is dead weight
rather than a missing door — flagging it would be crying wolf, which is the
one thing a checker must never do. (Under the same analysis the library has
13 unused entries, mostly a parallel live-attempt wrapper set the wall
bypasses by calling `dirtbag::` directly. Dead weight, not a bug.)

**`no-door` counts are printed on every preflight run**, so the eight
recorded here stay visible instead of quietly becoming permanent. That is
the difference between recorded debt and a silent hole, and this project has
now shipped six silent holes.

## What I would do about them

**None of these need the editor.** Every one is a prompt and a key on an
existing spot, the same shape as the fire's C/F and the shop's 1/2/3:

1. **Ethics** — the highest value for the least work. One key at the crag,
   and the consequences already flow to the HUD. It is also the system most
   likely to change how a career *feels*, because it is the only one that
   makes the valley think worse of you.
2. **The odd-jobs board** — the Shift spot already exists; it needs to offer
   the board rather than a generic shift.
3. **Sponsorship** — a prompt somewhere in town, gated on standing.
4. **Physio** — a town spot, costs money, buys back days.
5. **Gym membership** — a renewal prompt at the gym, once the gym is a place.

Ethics first, unless you want them in a different order.

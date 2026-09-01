# The book never got written into

**Found from a save file, day 8, seed `gym-1`.** Evan named his first ascent
`Bouncin` and then said: *"i didnt notice the name in game though so idk if
that was supposed to happen."*

It was supposed to happen. It didn't. The save proves the sim did its half:

```
project.0.name=the slab right of the pull-off
project.0.given=Bouncin
project.0.fa=1
project.0.confirmed=3
```

Two separate faults, both silent.

## 1. `NameFirstAscent` takes the crag line by const reference

```cpp
bool NameFirstAscent(ProjectMemory& memory, const CragLine& line,
                     const std::string& name);
```

It writes `memory.givenName`, `memory.firstAscent` and `memory.confirmedGrade`,
and by design never touches the line — `route.name` is the ledger key and
moving it would orphan every attempt already recorded against it. But nothing
else wrote the line either. `CragLine::displayName` existed, was documented as
"what the book shows", and was set by nobody, ever.

So the guidebook still said *project, the slab right of the pull-off, the book
guesses V3 and nobody has confirmed it* — about a line with a name, a first
ascent and a confirmed grade sitting in the ledger three fields away.

This also matters on reload. The crag is regenerated from the world seed every
time the venue changes and **none of it is saved**; the ascent lives only in
`ProjectMemory`. Even if naming had written into the book, walking to the cave
and back would have wiped it.

The fix is a sim function that rehydrates the page from the ledger:

```cpp
bool WriteIntoTheBook(CragLine& line, const ProjectMemory& memory,
                      const std::string& by);
```

Idempotent, keyed on `routeName`, and it sets four things: the given name, who
did it, `isProject = false`, and `route.grade = confirmedGrade` — the book's
guess becoming a fact, which is the entire payoff of the mechanic.

Called in two places engine-side, and it needs both:

- **`EnsureCrag`**, so the page is correct every time the book is built —
  this is what survives a reload.
- **`NameFirstAscent`**, so it is correct *now*. The player is standing in
  front of the thing they just named; waiting for the next venue change is
  not a fix.

`isProject = false` also closes the naming prompt properly: `CanName` wants an
open project, and after the write there isn't one.

## 2. The wall's name was a `BeginPlay` snapshot

`ADirtbagClimbWall::RouteName` was set once in `BeginPlay` from `Route.Name`
and read by the approach toast, the send toast and the HUD's route line. A
line named mid-session kept its old name on screen for the life of the actor,
even after the book was right.

`RefreshBookName()` re-reads the display name and grade from the book at
`BeginPlay` and on every approach. It touches only what is drawn — `Route.Name`
stays put, because it is still the key.

## 3. Nothing said the name back to you

The HUD has always drawn `FIRST ASCENT — <line> is yours to name`. There was
no other half. You typed a name, the widget closed, and the world carried on
as if nothing had happened.

`UDirtbagGameInstance::LastAscentLine` is set the moment a name is given — the
book's own line, `Bouncin  V3  FA you` — drawn in the same place and carried
for the rest of the day, then cleared at sleep. It is not saved: the ascent
lives in the ledger, and this is only the acknowledgement.

While wiring it, `FirstAscentLine` stopped hardcoding `"you"` and now signs
with `ClimberName` when there is one.

## Why the tests didn't catch it

`TestNamingIsEarnedAndExact` checks the ledger exhaustively and never looks at
the book, because until now nothing was supposed to write to the book. The
whole bug lived in the gap between two things that were each individually
correct. `TestTheBookGetsWrittenInto` closes it, and it asserts the negative
first — that naming alone leaves the page untouched — so the gap itself is
now written down as behaviour.

Verified by reintroducing the defect: commenting out `line.isProject = false`
fails 3 checks.

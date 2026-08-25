# When you get home — the whole list, in order

**Rewritten 2026-08-25 (fourth rewrite).** The previous version said **save
version v34**, thirteen checkers, forty sim files, and *"1,063 lines of
engine C++ written and never compiled."*

It is **v46**. Fourteen checkers. Forty-nine sim files. And the engine C++
that has never been near a compiler is **3,107 lines**, across twenty
commits.

Same lesson as the last three rewrites, and it is getting louder: **this
file goes stale faster than anything else in the repo, because it is the
only file whose job is to be current.**

---

## Read this bit before anything else

**§1 is the whole session's risk and it goes first.** Nothing below it
matters if the module does not build.

The sim half — now **27,585 lines** — is compiled here on every commit by
two compilers at two language standards, as forty-nine separate units and
again as one, and 162,978 assertions run against it. **None of that is a UHT
run and none of it is MSVC.** The engine module is **21,310 lines** and the
newest 3,107 of them have been written blind.

What was checked here, so you know what is already ruled out:

| checked | result |
|---|---|
| Sim under g++ and clang, C++17 **and C++20** (UE 5.8's standard) | clean, `-Wall -Wextra -Werror` |
| Sim as one translation unit, 49 files, the way UBT will | clean |
| `-Wshadow` across all of `Sim/` | zero |
| 162,978 harness assertions | pass |
| Seventeen preflight steps — fifteen checkers, the harness, the probe build | pass |
| The season probe still builds and runs | yes — **this is new, see §5** |
| New `UENUM`s are `uint8` | yes, all 8 |
| New `USTRUCT`s declared before first use | yes — `check-engine-fields` asserts declaration order across 85 mirror structs |
| Duplicate `UFUNCTION` names in one class | none |
| Every new Blueprint verb reachable from the game | yes — `check-doors` covers all 257 |
| Every sim field carried across the mirror | yes — `check-mirror-coverage`, 834 fields, 117 converters |
| Non-ASCII added in new code | none — **but see §2, which is about the old code** |

---

## §0 — One UHT error, found and fixed 2026-08-25

**You hit it and it was not from this week.** `DirtbagGameInstance.h(252):
Error: Found 'UENUM' while parsing UENUM`.

A `UENUM(BlueprintType)` had drifted **158 lines** from its `enum class
EDirtbagHandoverStep`, with a complete second UENUM in between. UHT read the
first macro, went looking for an enum, found another macro, and stopped.

It landed on **2026-08-22**, in "The handover: you could not end a career",
and it has two halves — the loud one that stopped your build, and a quiet
one that had been true for three days: **the declaration it belonged to had
no macro at all**, so `EDirtbagHandoverStep` was not reflected and nothing
said so.

Fixed, and there is now a checker: `tools/check-macros.py`, preflight step
sixteen, which asserts that between a reflection macro and its keyword there
may be blank lines and comments and nothing else — **both ways round**,
because only one of the two halves stops a build. Verified by reintroducing
the exact bug.

Behind it I also statically checked the other things UHT rejects that can be
seen from here — `UENUM(BlueprintType)` not `uint8`, a `USTRUCT`/`UCLASS`
with no `GENERATED_BODY()`, a `UPROPERTY` outside a body, a type used before
it is declared — and **all four are clean**. UHT stops at the first error,
so that is worth knowing before you rebuild: there is no second one of those
kinds waiting.

---

## §0b — Then MSVC found six, 2026-08-25

UHT passed after §0. The compiler found six, in four files, and **five of
the six were older than this week**:

| | |
|---|---|
| `Game->GymNameToBuy` | it is the *spot's* property, not the game instance's |
| `Printf("%s is yours.")` | knock-on from the above |
| `SpendDayWith(P, bClimbedToday)` | **the same missing argument that broke the probe** |
| `Line` hides `Line` | C4456, in the rival race |
| `Printf(bWasFa ? TEXT(..) : TEXT(..), ..)` | UE 5.8's format check is `consteval` |
| `Standing` hides `Standing` | C4456, in the HUD |

All six fixed. **Two are worth knowing about beyond the fix:**

**`SpendDayWith` was one bug that cost two build cycles.** The turnout commit
gave it a required `smell` argument and missed two call sites — the season
probe, which nothing here built, and the game instance, which nothing here
compiled. I found the probe on Monday and fixed it and did not think to ask
where else that function was called.

**The HUD shadow was not cosmetic.** Two locals called `Standing`; the inner
block fetched the *circuit* standing, tested it for emptiness, and then drew
the *faction* standing under it. It has been printing the wrong line under
the ranking for as long as both have existed. C4456 is what found it.

Three new preflight steps came out of this, and **all three were verified by
reintroducing the exact bug** — which is how I learned that two of them
looked like they worked and did not:

- `check-macros.py` — a reflection macro must sit on its declaration.
- `check-callsites.py` — every `dirtbag::` call in the engine must match the
  sim's parameter count, and every `FString::Printf` format must be a
  literal. Its first cut found the declarations with a pattern that
  forbade braces, which excluded every function defaulting a dial the way
  this project defaults dials — so the rule was live with almost nothing in
  its table. It reads 483 call sites now; it read 211 then.
- `check-ascii.py` — §2.

What is still **not** checked here, and would have caught two of today's
six: **shadowing in the engine module.** `-Wshadow` covers `Sim/` only,
because `Sim/` is the half that compiles here. I have not written a lexical
shadow checker because I do not trust one I cannot test against a compiler,
and MSVC already catches it in one build. If a third one turns up, it is
worth the risk.

---

## §1 — Build it, and expect UHT to be the thing that breaks

**Twenty-nine new `USTRUCT`/`UENUM`s and sixty-seven new `UFUNCTION`s**
since your last build. That is the largest single batch this project has
ever handed you, and UHT is the tool none of the fourteen checkers can
stand in for.

The new reflected types, in the order they will be parsed:

| | in |
|---|---|
| `EDirtbagGymPrice/SetMix/Equip/Campaign/Wing/Incident/Season/Regular` | `DirtbagSimTypes.h` |
| `EDirtbagLeaning`, `EDirtbagLeagueFormat` | same |
| `FDirtbagGym`, `FDirtbagGymStaffer`, `FDirtbagGymFloor` | same |
| `FDirtbagYouth`, `FDirtbagYouthKid`, `FDirtbagGraduate` | same |
| `FDirtbagGymLeague`, `FDirtbagLeagueChampion`, `FDirtbagLeagueStanding` | same |
| `FDirtbagBivy`, `EDirtbagSpot`, `FDirtbagLiving`, `FDirtbagLife`, `FDirtbagLocal` | same |

**The two shapes most likely to trip UHT**, because they are the two this
module had not used much before:

1. **`TArray<bool>`** — `FDirtbagGym::Wings`. Mirrors a `bool[5]`. If UHT
   objects, the fix is a `TArray<uint8>` or five named bools; the converter
   is four lines either way.
2. **A `USTRUCT` inside a `USTRUCT` inside a `USTRUCT`** —
   `FDirtbagPlayerState` → `FDirtbagGymLeague` → `TArray<FDirtbagLeagueChampion>`.
   Legal, but it is three deep and it is new.

Nine new bridge `.cpp` files were added to the module (`SimGym`,
`SimGymTown`, `SimGymFloor`, `SimGymLeague`, `SimYouth`, `SimBivy`,
`SimLiving`, `SimLife`, `SimLocals`). Each is a comment and one `#include`
of the sim `.cpp`. `check-engine-defs` asserts all forty-nine sim files have
one, so **if UBT reports an unresolved external for a `dirtbag::` symbol,
the bridge exists and the problem is elsewhere** — most likely a signature
that changed here and did not change in your local copy.

---

## §2 — The one real thing I found that I could not fix from here

**Non-ASCII characters — em dashes — are all over both halves of this
project, and not one file carrying them has a UTF-8 BOM.**

Measured precisely, because the first cut of this section undercounted it:

| | |
|---|---|
| files carrying non-ASCII at all | **79** of 248 |
| lines where it is inside a comment | 801 — harmless unless a mangled byte eats a newline |
| lines where it is **inside code or a string literal** | **28, across 18 files** |

The 28 are the ones that matter, and they are in both trees — `Sim/` is
compiled by MSVC too, through the bridge TUs, so this is not an engine-only
question:

```
DirtbagClimbWall.cpp:602   TEXT("%s  %s%s — %s   (E to climb, G for the book)")
DirtbagClimbWall.cpp:1270  TEXT("nothing to milk here — that cost you")
Sim/DirtbagConditions.cpp:214   "sticky — this is the day"
Sim/DirtbagAge.cpp:83           out += " — ";
```

Unreal's own convention is that a source file containing non-ASCII must be
saved **UTF-8 with BOM**, or MSVC reads it in the local codepage and the
literal is mangled. These files predate this week and the project evidently
builds, so one of two things is true:

- UBT is passing `/utf-8` and it has never mattered; or
- those lines have been rendering as `â€"` in game and nobody has looked
  closely at a route line.

**The test costs you ten seconds:** walk to a wall and read the route line
under the grade. If it says `—`, close this section. If it says anything
else, the fix is one pass converting `—` to `--` across the module, and I
will add a checker so it cannot come back.

Every string I wrote this week is ASCII (`--`, not `—`) in both halves, and
**`tools/check-ascii.py` now holds that line for everything written from
here** — it is preflight step fifteen. It grandfathers the 79 files by a
byte count, so a file may lose non-ASCII and may not gain it, which is the
honest way to hold a line without pretending to have cleaned up behind it.

If the answer comes back "they are mangled", the fix is one pass converting
every one to `--`, then `python3 tools/check-ascii.py --bless`, at which
point the numbers all go to zero and it becomes a flat ban.

---

## §3 — Save v46, and the twelve bumps behind it

`kSaveVersion` was **34** when you last built. It is **46**. Forty-five
migrations are in `DefaultMigrations()` and the harness asserts the count
equals `kSaveVersion - 1` on every run, so a bump without a migration cannot
ship.

| | |
|---|---|
| v35 | how well you ever knew somebody |
| v36 | a life outside climbing — the five threads |
| v37 | the people behind the counters |
| v38 | what you went to bed with (hunger carries) |
| v39 | the gym you bought |
| v40 | how you are living — grime, water, propane |
| v41 | where you park |
| v42 | the gym, pass two — staff as people, wings, incidents |
| v43 | the people in the building — the regulars and their arcs |
| v44 | the youth team |
| v45 | hosting the circuit |
| v46 | the league you run |

**Any save you have on disk from before this session will load.** That is
what the migration registry is for and there is a test per version. If one
does not, the message will be `LoadResult::BadFormat` and the field that is
missing is named in the reader.

---

## §4 — The input surface changed, and one spot changed a lot

The gym counter now has **eight pages behind six number keys**. Key 6
cycles; 1–5 mean different things per page:

```
the floor:        walk it (1)   comp night (2)
the levers:       price (1/2/3)   sets (4)   kit (5)
the people:       hire (1/2/3)   raise: yes (4) no (5)
the building:     wings (1-5)
the squad:        found it / session (1)   coach: you (2) hired (3)
the federation:   bid for the season (1)   run the round (2)
league night:     start one (1-4)   run tonight (5)
the keys:         hand it over (1)   the town (2)
```

**And an open incident overrides all eight** — while something is on the
clipboard, keys 1 and 2 answer it and nothing else is offered, because it is
the only thing here with a clock on it.

This is a lot of verbs on one counter and **I am not sure the paging is
right.** It was the honest way to fit thirty verbs behind six keys without
inventing an input mode, but a real panel would be better and this is a
judgement you can make in ten seconds at the desk and I cannot make at all.
The prompt text tells you which page you are on; if it reads badly, say so
and it becomes a widget.

No new input bindings are needed. Everything rides keys 1–6 and E, which are
already bound.

---

## §5 — What changed about the tooling, and why you should trust it more

**One preflight step is new, and one existing checker earned its keep
twice.** `tools/never-happened.py` is also new since your last build — it is
not part of preflight (it takes minutes, not seconds) and it answers the
opposite question to the others: *what rules does the game have that nothing
ever reaches.*

**New: `the probe still builds`.** `Sim/tools/season.cpp` had not compiled since
the turnout commit — `SpendDayWith` gained a required argument and the probe
was never updated — so **every measurement claim made between that commit
and this one could not have been re-run**, and preflight stayed green
throughout, because `check-parity.py` reads the probe's *source* and
`never-happened.py` runs a binary that was already on disk. Preflight builds
it now.

**`check-dials` caught two live ones this week**, having been in preflight
since before your last build: `YouthDials::gainHired`
read by nothing (the hired youth coach was being paid $30 a day to run
sessions that never happened — in the 2D source too), and two pairs of dials
sharing a name across structs that were genuinely different numbers.

`never-happened.py`'s battery grew from twenty-nine configurations to
thirty-four. Four of the five new ones exist because a whole system was
reporting dead for want of a policy that could reach it.

---

## §6 — Then Phase 6, which is the actual milestone and is all yours

Everything above is a build session. **Phase 6 — The Place — is the current
milestone and none of it can be done from a container**, which is why a week
of sim work has piled up in front of it.

The plan is `notes/phase6-the-place.md` and it has not changed: one level,
two tiers, blockout → walkable → dress, and **the Lot first**, because it is
where you sleep every night of a career and therefore the most-seen place in
the game by an order of magnitude.

What has changed is that **the Lot now has more to do in it than it did**.
Since the plan was written, the van got grime, water and propane; the night
got five places to park with a boot and a ticket if you overstay the free
one; the evening got five threads; and the town got people behind its
counters who greet you by name. All of that is sim and all of it is staged
in exactly the two places the plan says to build first.

**Done when** is unchanged: you can walk out of the van, drive to a crag,
climb, drive to town, work a shift and sleep — and every one of those
happens somewhere that looks like a place.

---

## What I would not do next

The gym family is finished — `GYM-1` through `GYM-12`, minus the two the
source never wrote — and it is the largest thing in the port that is not the
climbing itself. **Do not let me build another one before the level exists.**
`notes/the-2d-audit.md` still lists hitchhikers, the caravan, fishing, the
garden at the folks' farm and crafting as absent, and every one of them is
cheap sim work I can do blind, and none of them is what the game needs.

The three open balance calls, all yours and all logged:

- **Rapport does not discriminate careers** — 0.45 for a 99-day career and a
  2,085-day one (`notes/who-turns-up.md`).
- **The gym's regular arcs are spent in three weeks** of daily walking, and
  are the only relationship system in the port that ages on a click rather
  than a clock (`notes/who-the-gym-is-for.md`). One dial fixes it.
- **The circuit hosting deposit** was priced against a game whose careers
  are a few hundred days long; this one sees 156 seasons in thirty years
  (`notes/which-side-of-the-clipboard.md`).

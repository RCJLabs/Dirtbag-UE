# Sound

*2026-08-22. Phase 5, item 4.*

There was none. Not a cue, not an ambient, not an audio component —
`grep -r "Sound|Audio"` over the whole module returned the Epic template's
character files and nothing else, and `DirtbagUE/Content` had no audio asset
of any kind.

`concepts/DIRTBAG.md` §7 puts *"climbing sound design (chalk, breath, rubber
on rock)"* on the custom-work list, and those three are exactly the slots
built here.

## The split, stated up front

**This container cannot make a sound.** There is no editor to assign an
asset, no way to author one, and nothing to listen with. So the honest
division is:

- **Container-side:** where a cue fires, what the sim makes it do when it
  does, and the guarantee that the game is unchanged with every slot empty.
- **Desk-side:** the foley itself, and whether any of it sounds right.

Everything below is the first half. **The game ships from here silent**, and
that is the intended state, not an omission — which is why every play site
is guarded and why there is now a checker that a slot which *is* filled will
actually be heard.

## What fires, and what the sim does to it

A cue fired flat is a sound effect. A cue whose pitch and level come off the
same numbers the camera reads is staging — so every one of these takes its
parameters from the session, and the audio cannot drift away from what the
shot and the bars are saying.

| slot | when | what the sim does to it |
|---|---|---|
| `ChalkSound` | at a stance, **only** before a hard move | volume by how hard |
| `MoveSound` | each move | pitch varied per hold, deterministically |
| `SlipSound` | coming off | — |
| `LandSound` | hitting the mat | volume by how far you fell |
| `TopOutSound` | topping out | — |
| `BrushSound` | the brush | — |
| `BreathLoop` | the whole attempt | **volume and pitch by pump** |
| `InteractSound` | a day spot's E | one per spot |

### Chalk is gated, and that is the point

Nobody chalks for a jug. `ChalkBelowOdds = 0.7`, and it reads **the same
field the camera tightens on** — so the shot coming in and the chalk going
on say the same thing about the next move, one to the eye and one to the
ear. A cue that fires every move says nothing at all; the silence between
them is what makes one mean something.

### Breath is the one that matters

Pump is the resource the whole session turns on, and the player has been
reading it off a bar. Breath is how a pumped climber actually sounds, and it
is the ear's version of Phase 0's gate. It is a **loop** on an
`UAudioComponent` attached to the climber — so it goes up the route with
them — with volume rising from `BreathQuietVolume` to full and pitch to
`BreathPitchAtLimit = 1.22`. Modest on purpose: past about 1.3 a breath loop
reads as a chipmunk rather than as somebody in trouble.

It fades out over 0.6s rather than cutting, because an attempt ends with
somebody standing on the ground getting their breath back, not with the air
being switched off.

## One curve, not three

The camera sway had a bare `(pump/100)^2` typed into the engine. The breath
needed the same shape, and the animation would have wanted it next — three
guesses at one number, which is the thing dial discipline exists to prevent.

`dirtbag::PumpShows(pump)` is that curve, in the sim, with dials and tests:

- **`quietBelow = 35`** — below that, nothing shows. A watcher who can read
  pump off a fresh climber is reading a bar, which is the thing Phase 0's
  gate exists to make unnecessary.
- **`curve = 2.0`** — it comes on late and hard rather than creeping in.
  Halfway between quiet and spent scores under 0.35, and the last ten points
  of pump move it more than the ten points at 40 do.

The camera sway now reads it instead of its own square, so the eye and the
ear are driven by one number that can be tuned in one place.

### A test that passed on a bug

Reintroducing `curve = 1.0` failed two checks. Reintroducing `quietBelow =
0` — deleting the quiet zone entirely — **passed the whole suite.**

The reason is worth keeping: the quiet-zone checks were written as
`PumpShows(quietBelow) == 0` and `PumpShows(quietBelow - 10) == 0`, both of
which are trivially true when `quietBelow` is zero. **A check phrased in
terms of the dial it is checking cannot fail when that dial is wrong.**
They are pinned at absolute pump values now — 20 and 30 are silent, 45 is
not — and the reintroduction fires.

## Determinism, in the one place nobody would have looked

`MoveSound`'s pitch varies per hold so repeats do not sound identical, and
it varies off a hash of the hold index rather than off `FMath::Rand`. A
replay therefore sounds identical to the attempt it is replaying.

That is the same no-reroll discipline every gamble in this game lives under,
applied to the one system where nobody would ever have noticed it was
broken. It only has to be unpredictable to an ear, not to a statistician.

## The ninth checker

An `EditAnywhere TObjectPtr<USoundBase>` is a promise: drop an asset in and
you will hear it. If nothing ever plays the property, the slot is a lie —
and it is the **quietest possible bug**, because the symptom is that nothing
happens, which is also exactly what an unassigned slot looks like. There is
no way to tell them apart from inside the editor and no compiler would ever
complain.

`tools/check-cues.py` requires every `USoundBase` and `UAnimSequence` slot
to be named somewhere in its class's code besides its own declaration. It is
the written-and-never-wired check one layer out from `check-unwired.py`:
that one proves a sim declaration is reachable from the engine, this proves
an engine asset slot is reachable from its own code.

Verified by reintroduction on both types: deleting `PlayCue(TopOutSound)`
reports the sound slot, deleting `PlayAnim(FallAnim, …)` reports the anim
slot. It also audited the six pre-existing animation slots on the way past,
and all six are wired.

## One thing deliberately not built

**Ambient loops** — the fire crackling while you sit at it, the Lot at
night, wind at the Terrace. A spot fires when you press a key; an atmosphere
is somewhere you *are*, and that belongs to an `AmbientSound` actor placed in
the level next to the spot, not to a C++ property on it. Building it here
would have meant inventing a radius and a falloff for a place I cannot see.

## A compile risk removed on the way

The landing volume was first written with `FMath::GetMappedRangeValueClamped
(FVector2D, FVector2D, float)`. Under UE5's large-world coordinates
`FVector2D` is double-precision while that overload takes `FVector2f`, and
the conversion is the kind of thing that compiles on one engine version and
not the next. There is no compiler in this container to find out which, so
it is written long-hand instead. That is the whole reason `tools/preflight`
exists, applied by hand where no checker covers it.

## At the desk

**Nothing is assigned and nothing needs to be.** The game plays exactly as
it did. When you want to hear it:

1. The slots are on the wall actor under **Dirtbag|Sound**, and one per day
   spot under the same category.
2. **Start with `BreathLoop`.** It is the one that does the most for the
   least: a calm two-or-three-second breathing loop, and let the pitch and
   volume do the work. Everything else is garnish next to it.
3. Then `MoveSound` and `LandSound` — the two that fire most and make the
   climber feel like they weigh something.
4. `ChalkSound` last, and check it fires *rarely*. If you hear it every
   move, `ChalkBelowOdds` (0.7) is too high for that route's odds spread.
5. Fire crackle, wind, the Lot at night: place `AmbientSound` actors. Not
   these slots.

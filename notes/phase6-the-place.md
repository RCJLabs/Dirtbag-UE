# Phase 6 — the place

*2026-08-22. Written in answer to Evan: "i dont even know what assets to use
or how to put things together."*

Both halves of that get an answer below, and **the second half is the more
valuable one.** The shopping is a list; the assembly is the plan.

---

## Part 0: the audit nobody had done

### What you already own

| | what it solves |
|---|---|
| **ProceduralBuildingGenerator** (681 MB) | **the town block.** Already bought. |
| **PWL_Light_Manager** (235 MB) | lighting rig |
| **ClimbingAnimationSet** | the climber's poses on the wall |
| Epic Third Person template | locomotion, the pawn, the playground level |

Both Fab packs are gitignored (`SETUP.md` §3a) and reinstallable from the
launcher — that is deliberate and stays.

### The biggest line item on the shopping list is obsolete

`DIRTBAG.md` §7 leads its buy-don't-build list with **"Climbing systems (as
scaffolding)"** and three plugins: Procedural Climbing with Control Rig,
Climb and Vaulting Component V2, Dragon IK.

**Do not buy any of them.** CLAUDE.md's core design call retired that whole
category:

> *No physical climbing simulation, no hand-IK. The character climbs; the
> player drives the attempt with the 2D game's real-time verbs; the sim
> arbitrates everything.*

The climber is a mesh interpolated between spline points playing back a
handful of authored loops. `ADirtbagClimbWall` already has exactly six
animation slots — mount, hang idle, reach left, reach right, fall, top out —
and **`ClimbingAnimationSet`, which you already own, almost certainly fills
all six.** The largest saving available here comes from a decision made
months ago rather than from a purchase.

### And the list's other load-bearing entry has gone stale

§7 says *"Rock/terrain: Quixel Megascans — free for UE. Cliffs, boulders,
canyon surfaces solved."*

**Free Megascans ended 31 December 2024.** The library moved to Fab and
assets are individually priced now; a subset stays free, and Megaplants
(vegetation) are free. Still the right choice for the rock, and still worth
it — but it is a line item now, not a freebie, and the doc should stop
saying otherwise.

Two standing habits worth having: Fab's **Limited-Time Free** rotates every
two weeks and claiming is permanent, and Epic's own free-content library is
separate and always free.

---

## Part 1: what to buy

Given **stylized characters on real rock** and **buy what saves a week**.
Prices not quoted — I could not reach Fab directly from here to verify them,
and a stale price is worse than none.

**Buy, in this order:**

1. **A stylized modular character pack** with casual/civilian clothing. You
   need *you*, three named Lot regulars, and a handful of townspeople. A
   modular pack with swappable tops/legs/hair gets all of them out of one
   purchase — that is the whole reason to prefer modular over a set of
   fixed characters.
2. **Megascans cliff/boulder sets.** Two or three, deliberately different
   rock types, because crags 1–3 are *supposed* to look like different rock
   (`DIRTBAG.md` §6 rung 3 is literally "rock-type variety").
3. **A camper van with an interior.** Vehicle Variety Pack Vol. 2 includes a
   drivable campervan with an interior; there are standalone campervans too.
   You never drive it — travel is a fade and a teleport — so **interior
   quality matters and drivability does not.**
4. **The dog.** §7's DOG pack (26 animations including sniff, howl, rest,
   sleep) is still the right call. The emotional register is purchasable and
   you should purchase it.
5. **Epic's Game Animation Sample** — free, motion matching, 500+ ground
   animations. Everything that is not climbing.

**Do not buy:** climbing/IK plugins (above), a town pack (you own the
generator), a lighting pack (you own one).

---

## Part 2: how it goes together

This is the half you asked about and the half nobody has written down.

### One level. This is already decided, by the code — and it still holds.

`ADirtbagDaySpot::ArriveFromDrive` calls **`Pawn->TeleportTo`**. Travel is a
fade and a teleport inside a single level — not `OpenLevel`, not streaming.

So: **no sublevels, no World Partition data layers, no map-per-crag.** One
landscape, every location on it. That is not a preference; it is what the
game already does, and fighting it would be a rewrite for nothing.

### CORRECTION, same day: two tiers, not six pockets

The first version of this section said the six locations never need to be
geographically plausible relative to each other, because travel teleports.
**That is half right and the wrong half was load-bearing.** Evan:

> *"there are supposed to be attached zones for each location. the original
> dirtbag game that was made had zones connected except for crags and the
> olympics which you needed to use the van to get to. and then when
> traveling you saw the van moving across a background."*

CLAUDE.md is explicit that the 2D game is the spec, and the port had
departed from it without recording a decision. **The 2D game has two travel
rules; the port had one.**

| tier | places | how you get there | must they be coherent? |
|---|---|---|---|
| **connected ground** | the Lot, town/city | **on foot** | **yes — you can see one from the other, and that is the point** |
| **van destinations** | Roadside, the Cave, the Terrace, comps later | **the van, with a travel screen** | no — never visible from anywhere else |

So the pocket rule survives *for crags only*. The Lot and the town are one
continuous walkable place and have to look like one.

**The sim model for this is now built** (`Sim/DirtbagZones.h`, tested), and
it fixed a live bug on the way in — see below.

### The bug the correction exposed

Every travel spot in the build was gated on the van running. Under the 2D
game's rule only crags should be. The difference is most of a week of play:

- **Before:** a dead van cost you the crags, the gym, the gear shop, the
  diner and the shift. The van breaking took the *whole game* away.
- **After:** a dead van costs you the rock. You walk to town, work, and buy
  the part.

Being stranded from everything is not pressure, it is a pause. Being stuck
in town earning the money to fix it is a week of the game, and it is the
week the economy was designed to produce.

Travel spots carry a `DestinationZone` now. It **defaults to Roadside — a
crag — so every spot already placed behaves exactly as it does today** until
it is deliberately told it is a walk. A default that quietly made drives
free would be the worse direction to be wrong in.

### The travel screen does not exist and needs building

Today: fade to black, teleport, fade in. The 2D game: **the van crossing a
background, going somewhere.**

That is not decoration. It is the only time the van is ever on screen as a
vehicle rather than as a thing you sleep in and repair, and the van is what
the money is *for*. A fade says "time passed". A van crossing a background
says "this is what you keep alive."

Same shape as the sound work: the mechanism is container-side (a travel
screen, the clock running, the destination named, progress across it), the
background art is an asset slot. **Not built yet — it is the next thing.**

### One big city, or the town block?

Evan asked to consider it. **Recommendation: build the block first, and
build it so it can grow.**

The argument for the city is real — you own ProceduralBuildingGenerator, so
generating buildings is nearly free, and "one big city" is the kind of
decision that is expensive to retrofit.

The argument against is that the game currently has **four** things to do in
town: the gym, the gear shop, the diner, the job. A city of two hundred
buildings with four doors that open is worse than a block with four doors
that open — it reads as a film set, and every street you cannot enter is a
promise the game breaks. **The city earns its size when there is content to
put in it**, and that content does not exist and is not on the cut ladder.

The compromise costs nothing: lay the block out as a *corner* of a town
rather than as an island — streets that continue past the four doors and are
blocked plausibly, buildings that face a road that goes somewhere. Then
growing it later is generating more of what is already there, which is
exactly what the generator is for.

### Build them in that order, because that is the order of use

The Lot is where you sleep every night of a thirty-year career. It gets the
most screen time of anything in the game by an order of magnitude and it
should be finished first. The Cave and the Terrace are seasonal and can stay
grey boxes for months without hurting anything.

### And within each pocket: blockout → walkable → dress. Never dress first.

1. **Landscape actor, sculpted roughly.** Just enough to separate the
   pockets. Hills are occlusion, and occlusion is the whole job.
2. **Move the spots you already have onto it.** You have working trigger
   volumes on the playground template — that work is not thrown away, it
   gets *relocated*. This is the step that makes it a game in a place
   instead of a game in a test level.
3. **Play it.** Walk the Lot, sleep, drive to Roadside, climb. It will be
   grey and it will be a game.
4. **Then dress**, one pocket at a time, and play it again after each.

### Why the climb walls are safe to leave until last

`ADirtbagClimbWall` is a spline of hold positions plus a mesh that
interpolates along it. **The rock behind it is scenery.** A wall works on a
grey box and keeps working unchanged when a Megascans cliff arrives behind
it — you place the cliff, then drag the spline points onto features that
look like holds.

That means: **the rock purchase is not blocking anything.** You can build
the entire valley, play a full season in it, and buy cliffs afterwards.

### The one thing that is genuinely hard

**Making a hand-drawn hold spline line up with real rock features.** On a
grey box any spline works. On a photoscanned cliff, a hold that floats six
inches off the surface reads instantly as wrong.

Budget real time for this, do it on **one** wall first, and learn how much
fuss it is before committing to twenty-five. If it turns out to be brutal,
that is a finding worth having early — and the fallback (visible hold props
placed on the rock, spline snapped to them) is a known one.

---

## What this phase is not

Not more systems. The sim has twenty-six modules, a ninety-year headless
career, and nine preflight checkers. **That was never the risk** — §9 named
the risk correctly as *"session isn't tense/legible in 3D"* and §7 named the
craft centre correctly as the staging.

Trad is still the one unbuilt rung and it stays unbuilt. A deeper game with
no place to play it is a worse game, not a bigger one.

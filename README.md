# Dirtbag (Unreal reimagining)

The next RCJ Labs game: the climbing life-sim **Dirtbag**, reimagined in Unreal Engine. Live out of your van, work shifts, and push your grade from gym plastic to the crag — the 2D game's proven life-sim and session mechanics, staged in 3D.

**The core design call ("2D minigames, 3D staging"):** the character climbs; the player drives the attempt with the 2D game's real-time verbs (HOLD TO CLIMB, hold-to-load, the pump bar); the sim arbitrates. No physical climbing simulation, no hand-IK mountain — the watched session, the way LVDVS watches its fights.

## Where things are

- **[ROADMAP.md](ROADMAP.md)** — the live plan. Current milestone: **Phase 0 — Session Proof**.
- **[SETUP.md](SETUP.md)** — creating the UE project around the sim core (editor-side steps + Phase 0 shopping list).
- **`Sim/`** — the engine-free C++ sim core: seeded RNG (named streams), core types (grades, routes, climbers), and the per-move session resolver. Built and tested standalone: `Sim/run-tests.sh` (plain g++; the same translation units compile in the UE module).
- **`concepts/`** — the decision record: the 20-idea brainstorm shortlist, both finalists' concept docs and production plans, and the comparison that picked Dirtbag. **[BRAINSTORM.md](BRAINSTORM.md)** is the original 20-idea field.

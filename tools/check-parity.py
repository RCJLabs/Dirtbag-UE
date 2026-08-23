#!/usr/bin/env python3
"""The measured game and the played game must be the same game.

`Sim/tools/season.cpp` is the probe every balance number in this project
comes from. If it calls a sim rule the engine never calls, then **the
career being measured is not the career being played** — and every note
written from that probe describes a game nobody can reach.

That is not hypothetical. It happened three times in two days:

  WorkOddJob        the probe took gigs off the board, with their standing
                    effects; the engine called WorkShift, a flat wage with
                    no standing effects at all. Work and reputation were
                    joined in every measured career and disconnected in
                    every played one.

  GoToTheGym        checks IsGymMember. The probe called it; the engine
                    reached the gym by travel spot and wall, neither of
                    which asked, so the gym was free and the whole
                    membership system was decorative.

  ObligationToday   the probe took the sponsor's days; the engine asked
                    nobody, so a deal paid $640 a month and cost nothing.

Every one had the same shape: **a rule living in a sim function the engine
never called, with a reachable path around the side.** None of the other
checkers could see it — they check that declarations are reachable, not
that the two consumers of the sim agree about which rules apply.

Escape, with a reason:

    // probe-only: <why>   the probe legitimately uses it and the game
                           does not — a formatter for its own report, or a
                           filter the engine implements differently

Anything else is a divergence, and a divergence means the numbers lie.

## Verified against all three

Reintroducing each historical divergence fires it. The gym one fires as
`GoToTheGym` rather than as `IsGymMember` — and that is the correct name
for it, because `IsGymMember` was *always* reached: `KitLine` calls it, so
the HUD cheerfully printed **"gym membership: 12 days left"** the whole
time nothing enforced it. The gate was the thing missing, and the gate is
`GoToTheGym`.

## What this cannot see

It catches *the probe runs a rule the game never runs*. It cannot catch
*the game runs the rule and ignores the answer* — a divergence where the
engine computes something correctly and then does nothing with it would
pass here. That is a different check and it may not be greppable at all.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SIM = ROOT / "Sim"
PROBE = SIM / "tools" / "season.cpp"
MODULE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"

# The parameter list is matched lazily up to the first `)` that is followed
# by a semicolon, rather than with `[^;{]*`.
#
# The first version excluded `{` to avoid running past a function body, and
# that made it blind to every declaration with a braced default argument --
# `const DayDials& dials = DayDials{}` -- which is *most of this sim*. It
# saw 33 rules out of a couple of hundred and reported zero problems, and
# all three of the historical divergences it was written for were in the
# part it could not see. A checker that passes because it is not looking is
# worse than no checker.
DECL = re.compile(
    r"((?:^[ \t]*//[^\n]*\n)*)^[A-Za-z_][\w:<>,\s\*&]*?\b(\w+)\s*"
    r"\([^;]*?\)\s*;",
    re.M)


def call_count(text, name):
    """Occurrences that are calls rather than definitions, told apart by
    what follows the parameter list -- otherwise a sim function's own body
    in Sim/*.cpp would count as reaching itself."""
    calls = 0
    for m in re.finditer(r"\b" + re.escape(name) + r"\s*\(", text):
        i = m.end() - 1
        depth = 0
        while i < len(text):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        rest = re.sub(r"^const\s*", "", text[i + 1:i + 60].lstrip())
        if not rest.startswith("{"):
            calls += 1
    return calls


def sim_bodies():
    """Every sim function definition, mapped to its own body text.

    The sim is formatted with the closing brace of a definition at column
    zero, which is what makes this a scan rather than a parser."""
    out = {}
    for path in sorted(SIM.glob("*.cpp")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(
                r"^[A-Za-z_][\w:<>,\s\*&]*?\b(\w+)\s*\([^;]*?\)\s*\{",
                text, re.M):
            end = text.find("\n}", m.end())
            out[m.group(1)] = text[m.end():end if end > 0 else len(text)]
    return out


def main() -> int:
    probe = PROBE.read_text(encoding="utf-8", errors="replace")
    engine = "\n".join(
        p.read_text(encoding="utf-8", errors="replace")
        for p in sorted(MODULE.rglob("*.cpp"))
        if "Variant_" not in p.as_posix())

    # Reachability, properly, to a fixed point.
    #
    # A rule the engine never names directly can still be live, because the
    # sim is one connected thing driven through a few entry points:
    # FactionDay and KitDay are called inside SleepToNextDay, which the
    # engine calls every night.
    #
    # The first attempt at this counted *any* call from anywhere in the sim
    # as reach, and that was too generous by exactly one case -- it excused
    # IsGymMember, which is called from GoToTheGym, which is itself only
    # ever called by the probe. A rule reached only from a dead function is
    # not reached, and that was one of the three divergences this file
    # exists to catch. So: seed with what the engine calls directly, then
    # keep adding what the reached rules call, until nothing new appears.
    bodies = sim_bodies()
    reached = {n for n in bodies if call_count(engine, n) > 0}
    growing = True
    while growing:
        growing = False
        for name in list(reached):
            for callee in bodies:
                if callee in reached:
                    continue
                if call_count(bodies[name], callee) > 0:
                    reached.add(callee)
                    growing = True

    problems = 0
    checked = 0
    excused = 0

    for header in sorted(SIM.glob("*.h")):
        text = header.read_text(encoding="utf-8", errors="replace")
        for comment, name in DECL.findall(text):
            if call_count(probe, name) == 0:
                continue
            checked += 1
            if "probe-only:" in comment:
                excused += 1
                continue
            if name not in reached:
                problems += 1
                print(f"DIVERGENCE: {name}() is exercised by the probe and "
                      f"called by nothing in the engine.")
                print(f"            Every balance number measured through it "
                      f"describes a game nobody plays.")
                print(f"            Wire it, or mark the declaration in "
                      f"{header.name} `// probe-only: <why>`.")

    print(f"checked {checked} sim rules the probe exercises "
          f"({excused} probe-only); problems: {problems}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

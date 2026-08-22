#!/usr/bin/env python3
"""Every gameplay verb needs a door.

`UDirtbagGameInstance` is where this game's verbs live: take a shift, sign
with a sponsor, see a physio, do the thing you would not admit to. A verb
that is `BlueprintCallable` and that nothing calls is **a system with no
way in** -- it works, it is tested, it is saved and migrated, and the
player can never reach it.

This project has now found that bug at six layers and four of them were
exactly this shape:

  the fire had no verb           the campfire games could not be played
  dreams had no counter          the money had no destination you could pick
  the handover had no door       a whole phase, unreachable
  PeakGradeEver had no writer    a live function answering from constants

None of the existing checkers can see it. `check-unwired.py` proves a *sim*
declaration is reachable from the engine; this proves an *engine* verb is
reachable from play.

Scope is deliberately just the game instance. `DirtbagSimLibrary` exists to
expose the sim to Blueprint and an unused entry there is dead weight rather
than a missing door, so flagging it would be crying wolf -- which is the
one thing a checker must never do.

Two escapes, and both must give a reason:

  // blueprint-only: <why>   reached from a widget rather than from C++
  // no-door: <system>       a real gap, recorded on purpose

`no-door` counts are printed every run, so recorded debt stays visible
instead of quietly becoming permanent.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODULE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"
HEADER = MODULE / "DirtbagGameInstance.h"

FN = re.compile(
    r"((?:^[ \t]*//[^\n]*\n)*)[ \t]*UFUNCTION\(([^)]*)\)\s*\n"
    r"[ \t]*(?:static\s+|virtual\s+)?[\w:<>,\s\*&]+?\b(\w+)\s*\(", re.M)


def call_sites(text, name):
    """Occurrences of `name(` that are calls rather than definitions, told
    apart by what follows the parameter list: a definition is followed by
    an opening brace."""
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


def main() -> int:
    body = "\n".join(
        p.read_text(encoding="utf-8", errors="replace")
        for p in sorted(MODULE.rglob("*.cpp"))
        if "Variant_" not in p.as_posix())
    text = HEADER.read_text(encoding="utf-8", errors="replace")

    problems = 0
    checked = 0
    recorded = []

    for comment, spec, name in FN.findall(text):
        if "Blueprint" not in spec:
            continue
        checked += 1
        if "blueprint-only:" in comment:
            continue
        if "no-door:" in comment:
            reason = comment.split("no-door:")[1].split("\n")[0].strip()
            recorded.append(f"{name} ({reason})")
            continue
        if call_sites(body, name) == 0:
            problems += 1
            print(f"NO DOOR: {name}() is Blueprint-exposed and nothing calls "
                  f"it.")
            print(f"         Wire it into the game, or say why not above the "
                  f"UFUNCTION with")
            print(f"         `// blueprint-only: <why>` or "
                  f"`// no-door: <system>`.")

    print(f"checked {checked} game-instance verbs; problems: {problems}")
    if recorded:
        print(f"  {len(recorded)} recorded as having no door yet:")
        for r in sorted(recorded):
            print(f"    - {r}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

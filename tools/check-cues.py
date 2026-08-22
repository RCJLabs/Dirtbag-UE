#!/usr/bin/env python3
"""Asset slots must actually be used.

An EditAnywhere TObjectPtr<USoundBase> or TObjectPtr<UAnimSequence> is a
promise to whoever is holding the editor: drop an asset in this slot and
you will hear it, or see it. If nothing in the class ever plays the
property, the slot is a lie -- and it is the *quietest* possible bug,
because the symptom is that nothing happens, which is also exactly what an
unassigned slot looks like. There is no way to tell the two apart from
inside the editor, and no compiler here would ever complain.

This is the same written-and-never-wired class the sim checker covers, one
layer out: check-unwired.py proves a sim declaration is reachable from the
engine, and this proves an engine asset slot is reachable from its own
code.

The rule: a property of one of these types must be named somewhere in the
class's .cpp besides its own declaration.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODULE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"

# The asset types whose whole purpose is "assign one and something
# happens". Add to this list when a new one earns it.
ASSET_TYPES = ("USoundBase", "UAnimSequence")

PROP = re.compile(
    r"TObjectPtr<\s*(?:class\s+)?(" + "|".join(ASSET_TYPES) + r")\s*>\s+(\w+)\s*;")


def main() -> int:
    problems = 0
    checked = 0

    for header in sorted(MODULE.rglob("*.h")):
        if "Variant_" in header.as_posix():
            continue
        text = header.read_text(encoding="utf-8", errors="replace")
        props = PROP.findall(text)
        if not props:
            continue

        cpp = header.with_suffix(".cpp")
        body = cpp.read_text(encoding="utf-8", errors="replace") if cpp.exists() else ""
        # A property may also be used from the header itself (an inline
        # accessor), so both count.
        haystack = body + "\n" + text

        for kind, name in props:
            checked += 1
            # Every mention except the declaration itself.
            uses = len(re.findall(r"\b" + re.escape(name) + r"\b", haystack))
            declarations = len(
                re.findall(r"TObjectPtr<\s*(?:class\s+)?\w+\s*>\s+"
                           + re.escape(name) + r"\s*;", haystack))
            if uses - declarations <= 0:
                problems += 1
                print(f"NEVER USED: {header.name}'s {kind} slot '{name}' is "
                      f"declared and never played.")
                print(f"            Assigning an asset to it in the editor "
                      f"would do nothing, and nothing would say so.")

    print(f"checked {checked} asset slots across the module "
          f"({', '.join(ASSET_TYPES)}); problems: {problems}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Asset slots must actually be used.

An EditAnywhere TObjectPtr<USoundBase>, <UAnimSequence> or <UTexture2D> is
a promise to whoever is holding the editor: drop an asset in this slot and
you will hear it, or see it. If nothing in the class ever plays the
property, the slot is a lie -- and it is the *quietest* possible bug,
because the symptom is that nothing happens, which is also exactly what an
unassigned slot looks like. There is no way to tell the two apart from
inside the editor, and no compiler here would ever complain.

This is the same written-and-never-wired class the sim checker covers, one
layer out: check-unwired.py proves a sim declaration is reachable from the
engine, and this proves an engine asset slot is reachable from its own
code.

The rule: a property of one of these types must be *read* somewhere in the
module besides its own declaration.

## What this does not catch, stated plainly

It counts reads, not draws. `if (T.Backdrop)` is a read, so a slot that is
null-guarded and then never actually drawn still passes. Telling those
apart needs a real parser, and this is a grep.

Both limits were found by reintroduction rather than assumed: deleting the
`DrawTexture(T.Backdrop, ...)` call leaves the guard behind and the checker
stays quiet. What it *does* catch reliably is the common and much more
likely case -- an EditAnywhere slot that nothing in the module mentions at
all, which is what happens when a property is added and the call site is
forgotten. Both `TopOutSound` and `FallAnim` fire on that.

Assignments are excluded, and that mattered: a readout struct's field is
always written by the producer -- the travel spot fills `T.Backdrop` before
the HUD ever sees it -- so counting any mention proved only "somebody fills
this", which is exactly the half that is never the bug.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODULE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"

# The asset types whose whole purpose is "assign one and something
# happens". Add to this list when a new one earns it.
ASSET_TYPES = ("USoundBase", "UAnimSequence", "UTexture2D")

PROP = re.compile(
    r"TObjectPtr<\s*(?:class\s+)?(" + "|".join(ASSET_TYPES) + r")\s*>\s+(\w+)\s*;")


def main() -> int:
    problems = 0
    checked = 0

    module_text = "\n".join(
        p.read_text(encoding="utf-8", errors="replace")
        for p in sorted(MODULE.rglob("*"))
        if p.suffix in (".h", ".cpp") and "Variant_" not in p.as_posix())

    for header in sorted(MODULE.rglob("*.h")):
        if "Variant_" in header.as_posix():
            continue
        text = header.read_text(encoding="utf-8", errors="replace")
        props = PROP.findall(text)
        if not props:
            continue

        # The whole module, not just the paired .cpp.
        #
        # The first version looked only at `Foo.h` + `Foo.cpp` and
        # immediately produced a false positive on the travel readout's
        # backdrop: readout structs are declared on the game instance,
        # filled by a spot and drawn by the HUD, which is the entire
        # pattern this project's presentation layer is built on. "Reachable
        # from anywhere in the module" is the honest question -- it is a
        # weaker check than per-class, and a weaker check that is right
        # beats a stronger one that cries wolf.
        haystack = module_text

        for kind, name in props:
            checked += 1
            # Mentions that are *reads*, minus the declaration itself.
            #
            # Assignments do not count, and finding that out took two
            # reintroductions that should have failed and did not. A readout
            # struct's field is always written by the producer -- the travel
            # spot fills `T.Backdrop` before the HUD ever sees it -- so
            # counting any mention proved only "somebody fills this", which
            # is exactly the half that is never the bug. Deleting the
            # DrawTexture call left the slot silently undrawn and the
            # checker happy.
            uses = len(re.findall(r"\b" + re.escape(name) + r"\b", haystack))
            writes = len(
                re.findall(r"\b" + re.escape(name) + r"\s*=(?!=)", haystack))
            declarations = len(
                re.findall(r"TObjectPtr<\s*(?:class\s+)?\w+\s*>\s+"
                           + re.escape(name) + r"\s*;", haystack))
            if uses - writes - declarations <= 0:
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

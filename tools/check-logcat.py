#!/usr/bin/env python3
"""Log categories must not be defined twice in one module.

UBT unity-builds by default: it concatenates several .cpp files into one
translation unit. Two files that each write

    DEFINE_LOG_CATEGORY_STATIC(LogDirtbagSetup, Log, All);

compile perfectly on their own and collide the instant UBT stitches them
together -- a redefinition error, on Evan's machine, in a file neither of
the two authors was looking at.

That is exactly the mistake this checker was written for: the toast triage
added a shared setup-log category to both DirtbagClimbWall.cpp and
DirtbagDaySpot.cpp, each with its own DEFINE_LOG_CATEGORY_STATIC. Nothing
in the container could compile it, every other checker passed, and the
build cycle would have been lost to two lines that looked obviously right.

The rule: a category name may be DEFINE'd in at most one .cpp. A name
shared between files must be DECLARE_LOG_CATEGORY_EXTERN'd in a header and
DEFINE_LOG_CATEGORY'd once.
"""

import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODULE = ROOT / "DirtbagUE" / "Source"

DEFINE = re.compile(
    r"^\s*DEFINE_LOG_CATEGORY(?:_STATIC)?\s*\(\s*([A-Za-z_]\w*)", re.M)
DECLARE = re.compile(
    r"^\s*DECLARE_LOG_CATEGORY_EXTERN\s*\(\s*([A-Za-z_]\w*)", re.M)
USE = re.compile(r"\bUE_(?:LOG|CLOG)\s*\(\s*([A-Za-z_]\w*)")


def main() -> int:
    defined = defaultdict(list)   # name -> [cpp paths]
    declared = {}                 # name -> header path
    used = defaultdict(list)      # name -> [paths that UE_LOG to it]

    for path in sorted(MODULE.rglob("*")):
        if path.suffix not in (".cpp", ".h"):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for name in set(USE.findall(text)):
            used[name].append(path)
        if path.suffix == ".cpp":
            for name in DEFINE.findall(text):
                defined[name].append(path)
        else:
            for name in DECLARE.findall(text):
                declared.setdefault(name, path)

    problems = 0
    for name, paths in sorted(defined.items()):
        if len(paths) > 1:
            problems += 1
            where = ", ".join(p.relative_to(ROOT).as_posix() for p in paths)
            print(f"DEFINED TWICE: {name} in {where}")
            print("               a unity build puts these in one translation "
                  "unit and the second is a redefinition.")
            print("               Declare it once in a header with "
                  "DECLARE_LOG_CATEGORY_EXTERN and define it once.")

    # The other half of the same mistake: a category that something logs
    # to, declared in a header, and defined in no .cpp -- which links to
    # nothing.
    #
    # Use-aware on purpose. The first version of this check flagged any
    # declared-but-undefined category and immediately caught two of Epic's
    # own template headers, which declare LogTemplateCharacter and
    # LogCombatCharacter and never log to them. Those link perfectly well:
    # an extern nobody references costs nothing. Flagging them would have
    # meant a whitelist, and a checker with a whitelist is a checker people
    # learn to ignore.
    for name, header in sorted(declared.items()):
        if name in defined or name not in used:
            continue
        problems += 1
        where = ", ".join(p.relative_to(ROOT).as_posix() for p in used[name])
        print(f"NEVER DEFINED: {name} is declared in "
              f"{header.relative_to(ROOT).as_posix()}, logged to in {where}, "
              f"and defined in no .cpp -- that is a link error.")

    print(f"checked {len(defined)} log categories defined across the module "
          f"({len(declared)} declared in headers); problems: {problems}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

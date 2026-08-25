#!/usr/bin/env python3
"""Engine sources must not gain new non-ASCII characters.

Unreal's convention is that a source file containing non-ASCII be saved
UTF-8 **with a BOM**, or MSVC reads it in the local codepage and every
literal in it is mangled. Eighteen files in this module contain em dashes
and not one has a BOM (`notes/NEXT-AT-THE-DESK.md` §2), including
user-facing `TEXT()` literals:

    DirtbagClimbWall.cpp   TEXT("nothing to milk here — that cost you")

Those predate this checker and the project builds, so either UBT is passing
`/utf-8` and it has never mattered, or those lines render mangled in game.
**This checker does not decide that** -- it is one look at a route line at
the desk, and it is Evan's.

What it does is make the answer not matter for anything written from here:
ASCII is safe under both outcomes, so new code stays ASCII. The existing
files are grandfathered by a byte count, which is the honest way to hold a
line without pretending to have cleaned up behind it -- a file may lose
non-ASCII and may not gain it.

If the answer comes back "they are mangled", the fix is one pass converting
every one to `--` and then dropping the grandfather table, at which point
this becomes a flat ban and the numbers all go to zero.

Sim/ is checked the same way and for a different reason: those files are
compiled by MSVC too, through the bridge TUs.

Opt out for one file by adding it to ALLOWED with a count, which is what a
deliberate change of an existing string looks like.
"""
import io
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TREES = [ROOT / "DirtbagUE" / "Source" / "DirtbagUE", ROOT / "Sim"]

# Files that already contain non-ASCII, and how many bytes of it. A file may
# come down from its number; it may not go up. Regenerate with --bless after
# a deliberate cleanup.
GRANDFATHERED = ROOT / "tools" / "ascii-baseline.txt"


def offenders(path):
    raw = io.open(path, "rb").read()
    return sum(1 for b in raw if b > 0x7F)


def collect():
    out = {}
    for tree in TREES:
        for path in sorted(tree.rglob("*")):
            if path.suffix not in (".h", ".cpp"):
                continue
            if "tests" in path.parts:
                continue
            n = offenders(path)
            if n:
                out[str(path.relative_to(ROOT))] = n
    return out


def baseline():
    if not GRANDFATHERED.exists():
        return {}
    out = {}
    for line in io.open(GRANDFATHERED, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, _, count = line.rpartition(" ")
        out[name] = int(count)
    return out


def main():
    found = collect()
    if "--bless" in sys.argv:
        with io.open(GRANDFATHERED, "w", encoding="utf-8") as fh:
            fh.write("# Non-ASCII bytes per file, frozen. A file may lose "
                     "them; it may not gain them.\n")
            fh.write("# See tools/check-ascii.py. Regenerate with --bless "
                     "after a deliberate cleanup.\n")
            for name in sorted(found):
                fh.write("%s %d\n" % (name, found[name]))
        print("blessed %d files" % len(found))
        return 0

    was = baseline()
    problems = []
    for name in sorted(found):
        allowed = was.get(name, 0)
        if found[name] > allowed:
            problems.append(
                "%s: %d non-ASCII bytes, was %d. New code must be ASCII -- "
                "use `--` for an em dash. See tools/check-ascii.py."
                % (name, found[name], allowed))
    # A file that lost its non-ASCII is good news and the baseline should
    # follow it down, so say so rather than staying quiet about it.
    cleaned = [n for n in was if found.get(n, 0) < was[n]]

    for line in problems:
        print("  " + line)
    for name in sorted(cleaned):
        print("  %s is cleaner than the baseline (%d -> %d). Re-bless when "
              "you are done." % (name, was[name], found.get(name, 0)))
    print("checked %d engine and sim sources; %d carry non-ASCII; problems: %d"
          % (sum(1 for t in TREES for p in t.rglob("*")
                 if p.suffix in (".h", ".cpp") and "tests" not in p.parts),
             len(found), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

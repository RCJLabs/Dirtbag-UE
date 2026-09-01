#!/usr/bin/env python3
"""A dynamic delegate target must be a UFUNCTION.

This exists because of a specific PIE session. The log said:

    Ensure condition failed: this->IsBound()
    Unable to bind delegate to 'OnApproachBegin' (function might not be
    marked as a UFUNCTION or object may be pending kill)
    ADirtbagClimbWall::BeginPlay() [DirtbagClimbWall.cpp:256]

`AddDynamic` resolves its target by **name**, at runtime, through the
reflection tables. A target with no `UFUNCTION()` over it compiles clean,
links clean, and then simply is not there: the delegate binds to nothing
and every overlap it was meant to catch is silently dropped. In that case
the wall's approach trigger had never once fired -- `OnApproachEnd` had its
macro and `OnApproachBegin` did not, so walking up to a wall did nothing
and walking away from it worked.

None of the other checkers could see it. `check-macros` asks whether a
reflection macro sits on its declaration, but only for types; a UFUNCTION
on a method is a different table and nothing was reading it. This one takes
the other end: start from the call sites, which name every function that
has to be reflected, and demand the macro at the declaration.

The check is worth having precisely because the failure is quiet. A missing
UCLASS costs a build; a missing UFUNCTION costs an evening of wondering why
a trigger volume does nothing.
"""
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TREE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"

# `X->Y.AddDynamic(this, &AClass::Method);` -- \s covers the line breaks the
# formatter puts after the open paren.
CALL = re.compile(r"AddDynamic\s*\(\s*[^,]+,\s*&\s*(\w+)\s*::\s*(\w+)\s*\)")
# The class this header declares. `class DIRTBAGUE_API AFoo : public ...`.
CLASS = re.compile(r"^\s*class\s+(?:\w+_API\s+)?(\w+)\s*(?::|$|\{)")
SKIP = re.compile(r"^\s*(//|/\*|\*|$)")


def headers():
    """class name -> (path, lines)."""
    found = {}
    for path in sorted(TREE.rglob("*.h")):
        lines = io.open(path, encoding="utf-8", errors="replace").read().split("\n")
        for line in lines:
            m = CLASS.match(line)
            if m:
                found.setdefault(m.group(1), (path, lines))
    return found


def declared(lines, method):
    """Line index of `method`'s declaration, or None."""
    want = re.compile(r"^\s*(?:virtual\s+)?\w[\w:<>,\s\*&]*?\b%s\s*\(" % method)
    for i, line in enumerate(lines):
        if want.match(line):
            return i
    return None


def main():
    by_class = headers()
    problems = []
    sites = 0
    files = sorted(TREE.rglob("*.cpp"))
    for path in files:
        text = io.open(path, encoding="utf-8", errors="replace").read()
        for m in CALL.finditer(text):
            sites += 1
            cls, method = m.group(1), m.group(2)
            where = "%s:%d" % (path.name, text[:m.start()].count("\n") + 1)
            if cls not in by_class:
                problems.append(
                    "%s  AddDynamic names %s::%s but no header declares "
                    "class %s." % (where, cls, method, cls))
                continue
            hpath, lines = by_class[cls]
            at = declared(lines, method)
            if at is None:
                problems.append(
                    "%s  AddDynamic names %s::%s but %s declares no such "
                    "method." % (where, cls, method, hpath.name))
                continue
            # Walk back past comments and blank lines only.
            j = at - 1
            while j >= 0 and SKIP.match(lines[j]):
                j -= 1
            if j < 0 or "UFUNCTION" not in lines[j]:
                problems.append(
                    "%s  %s::%s is bound with AddDynamic but %s:%d declares "
                    "it without UFUNCTION(). It will bind to nothing and the "
                    "delegate will never fire."
                    % (where, cls, method, hpath.name, at + 1))
    for line in problems:
        print("  " + line)
    print("checked %d AddDynamic call sites in %d files; problems: %d"
          % (sites, len(files), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""A reflection macro must sit directly on top of the thing it reflects.

This exists because of a specific build cycle. UHT said:

    DirtbagGameInstance.h(252): Error: Found 'UENUM' while parsing UENUM

and the cause was a `UENUM(BlueprintType)` whose `enum class` had drifted
**158 lines down the file**, with another complete UENUM in between. UHT
read the first macro, went looking for an enum, found a second macro
instead, and stopped. The declaration it belonged to had no macro at all
and would have been invisible to Blueprint even if the parse had survived.

None of the other checkers could see it. `check-engine-defs` asserts a
declaration exists; `check-engine-fields` asserts field names and struct
order; neither asks whether a macro and its declaration are still adjacent.
This one does, and it is the cheapest possible parse: between a reflection
macro and its keyword there may be blank lines and comments and **nothing
else**.

Checked both ways round, because the failure has two halves and only one of
them stops the build:

  1. A macro with no declaration under it -- UHT errors, which is loud.
  2. A `UCLASS`/`USTRUCT`/`UENUM`-shaped declaration with no macro over it
     -- UHT says nothing at all and the type is simply not reflected. That
     is the half that ate `EDirtbagHandoverStep` silently for three days.

Rule 2 needs a way to say "this one is deliberately a plain type", which is
most structs in a header. It only fires on a declaration that **used to**
have a macro, i.e. one this file lists in PAIRED. Anything else is assumed
plain, which is the safe direction: a plain type that should be reflected
shows up as a missing Blueprint node the moment you look for it, whereas a
stranded macro costs a build.
"""
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TREE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"

MACRO = re.compile(r"^\s*(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(")
# What each macro must be followed by. **Strictly the keyword** -- the first
# cut of this table also allowed another macro, which permits precisely the
# thing UHT rejects ("Found 'UENUM' while parsing UENUM") and left rule 1
# unable to fire on the bug it was written for. Caught by reintroducing the
# bug and watching the wrong rule catch it.
WANTS = {
    "UCLASS": re.compile(r"^\s*class\b"),
    "UINTERFACE": re.compile(r"^\s*class\b"),
    "USTRUCT": re.compile(r"^\s*struct\b"),
    "UENUM": re.compile(r"^\s*enum\b"),
}
# A declaration of a reflected shape, for rule 2.
DECL = re.compile(r"^\s*(enum\s+class|struct)\s+(F?E?Dirtbag\w+)")

SKIP = re.compile(r"^\s*(//|/\*|\*|$)")


def unreflected(path):
    """Names declared with no macro above them, and macros with no decl."""
    lines = io.open(path, encoding="utf-8", errors="replace").read().split("\n")
    stranded = []
    plain = []
    for i, line in enumerate(lines):
        m = MACRO.match(line)
        if m:
            # Walk forward past comments and blank lines only.
            j = i + 1
            while j < len(lines) and SKIP.match(lines[j]):
                j += 1
            if j >= len(lines) or not WANTS[m.group(1)].match(lines[j]):
                found = lines[j].strip()[:60] if j < len(lines) else "end of file"
                stranded.append(
                    "%s:%d  %s( with no %s under it -- found %r. UHT reads "
                    "this as 'Found X while parsing X'."
                    % (path.name, i + 1, m.group(1), m.group(1).lower()[1:],
                       found))
            continue
        d = DECL.match(line)
        if d:
            # Walk backwards past comments and blank lines only.
            j = i - 1
            while j >= 0 and SKIP.match(lines[j]):
                j -= 1
            if j < 0 or not MACRO.match(lines[j]):
                plain.append((d.group(2), i + 1))
    return stranded, plain


# Types that are deliberately plain C++ -- no macro, not reflected, and that
# is correct. Regenerate with --bless after adding one on purpose.
PLAIN = ROOT / "tools" / "plain-types.txt"


def blessed():
    if not PLAIN.exists():
        return set()
    return {l.split()[0] for l in io.open(PLAIN, encoding="utf-8")
            if l.strip() and not l.startswith("#")}


def main():
    stranded, plain = [], []
    files = sorted(TREE.rglob("*.h"))
    for path in files:
        s, p = unreflected(path)
        stranded.extend(s)
        plain.extend((n, path.name, ln) for n, ln in p)

    if "--bless" in sys.argv:
        with io.open(PLAIN, "w", encoding="utf-8") as fh:
            fh.write("# Types declared without a reflection macro on "
                     "purpose. See tools/check-macros.py.\n")
            for name, fname, ln in sorted(plain):
                fh.write("%s  # %s:%d\n" % (name, fname, ln))
        print("blessed %d plain types" % len(plain))
        return 0

    ok = blessed()
    newly = [(n, f, l) for n, f, l in plain if n not in ok]
    for line in stranded:
        print("  " + line)
    for name, fname, ln in newly:
        print("  %s:%d  %s is declared with no reflection macro over it. If "
              "that is deliberate, run --bless; if not, UHT is silently not "
              "reflecting it." % (fname, ln, name))
    problems = len(stranded) + len(newly)
    print("checked %d headers for stranded reflection macros; problems: %d"
          % (len(files), problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

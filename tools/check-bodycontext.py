#!/usr/bin/env python3
"""Every attempt is climbed by a body.

`AttemptInput` is the resolver's whole world: who is climbing, on what, how
warm, how padded, how worn the rubber, and what is wrong with them. Some of
those fields are about the *situation* and belong to whoever is staging the
attempt. The rest are about the **body**, they follow a climber everywhere,
and there is exactly one function that stamps them on: `ApplyBody`.

This checker exists because a construction site skipped it and nothing
noticed.

`AttemptProblem` -- the comp resolver -- built its own `AttemptInput` and
set six fields. It never set the joints, the ailments, the comeback stage,
the flaw or the rubber, so a climber with a maxed-out finger joint, four
cortisone shots, a bad flu and an abscess scored **identically** to a
healthy one at a comp:

    comp score, healthy climber:      4.1
    comp score, same climber wrecked: 4.1

Two whole phases were invisible in the path that serves gym comps, the
circuit, the World Cup, the Games and league nights -- most of the game's
indoor time.

No existing checker could see it. `check-unwired.py` proves a sim
declaration is *reachable*; `check-doors.py` proves an engine verb is
*callable*. Neither proves that a caller passed everything the callee
needed, because both of those are questions about names and this is a
question about a struct being filled in.

The rule: **any function that declares a local `AttemptInput` must call
`ApplyBody` on it.** One escape, and it must give a reason:

    // body-ok: <why>        this input is deliberately bodiless
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCAN = [ROOT / "Sim", ROOT / "DirtbagUE" / "Source"]
SKIP_DIRS = {"tests", "tools"}

DECL = re.compile(r"^\s*AttemptInput\s+(\w+)\s*;")
ESCAPE = re.compile(r"//\s*body-ok:\s*\S")


def strip_comments(text):
    """Comments are prose in this project and full of the words we match."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return "\n".join(re.sub(r"//.*", "", line) for line in text.split("\n"))


def enclosing_function(lines, at):
    """Walk back to the line that opens whatever this is inside.

    Crude on purpose: the nearest preceding line at column zero that is not
    a brace, a preprocessor line or a namespace. That is enough here, and a
    cleverer parser would be a C++ parser.

    `struct` and `class` come back rather than being skipped, because a
    **field** of type `AttemptInput` is not a build site -- it holds one
    that was stamped somewhere else -- and a checker that flags every field
    would be crying wolf, which is the one thing a checker must never do.
    """
    for i in range(at, -1, -1):
        line = lines[i]
        if not line or line[0] in " \t}#":
            continue
        if line.startswith(("namespace", "using", "//")):
            continue
        return i
    return 0


def function_end(lines, start):
    depth = 0
    seen = False
    for i in range(start, len(lines)):
        depth += lines[i].count("{") - lines[i].count("}")
        if "{" in lines[i]:
            seen = True
        if seen and depth <= 0:
            return i
    return len(lines) - 1


def main():
    problems = []
    sites = 0
    escaped = 0

    files = []
    for root in SCAN:
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            if any(part in SKIP_DIRS for part in path.parts):
                continue
            files.append(path)

    for path in files:
        raw = path.read_text(encoding="utf-8", errors="replace")
        raw_lines = raw.split("\n")
        lines = strip_comments(raw).split("\n")

        for n, line in enumerate(lines):
            m = DECL.match(line)
            if not m:
                continue
            start = enclosing_function(lines, n)
            if lines[start].startswith(("struct", "class")):
                continue   # a field, not a build site
            sites += 1
            end = function_end(lines, start)
            window = "\n".join(lines[start:end + 1])
            # The escape is read off the *raw* text, because it is a comment
            # and the stripped copy has thrown it away.
            raw_window = "\n".join(raw_lines[max(0, start - 3):end + 1])
            if ESCAPE.search(raw_window):
                escaped += 1
                continue
            if "ApplyBody(" in window:
                continue
            rel = path.relative_to(ROOT)
            problems.append(
                f"{rel}:{n + 1}: `AttemptInput {m.group(1)}` is built here "
                f"and never stamped with ApplyBody().\n"
                f"    An attempt assembled by hand is an attempt climbed by "
                f"nobody in particular -- no flaw, no\n"
                f"    joints, no flu, no rubber. Call ApplyBody(), or say why "
                f"not with `// body-ok: <why>`."
            )

    print(f"checked {sites} AttemptInput build sites "
          f"({escaped} deliberately bodiless); problems: {len(problems)}")
    for p in problems:
        print("  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

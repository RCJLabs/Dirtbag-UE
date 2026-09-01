#!/usr/bin/env python3
"""Engine call sites must match the sim declarations they call.

Two rules, and both exist because of a specific build cycle on 2026-08-25.

**1. Arity.** `SpendDayWith` gained a required `smell` argument in the
turnout commit and **two** call sites were never updated: the season probe,
which stopped compiling silently for four days because nothing built it, and
`DirtbagGameInstance.cpp`, which stopped compiling on Evan's PC and nobody
knew until he pressed build. `check-engine-defs` asserts a declaration
exists and `check-mirror-coverage` asserts fields are carried; neither reads
an argument list. This does.

A sim function's declaration gives a range -- required parameters at the
bottom, defaulted ones at the top -- and a call outside that range is a
compile error waiting on somebody else's machine. Overloads widen the range
rather than being resolved, which is the safe direction: this checker is
here to catch "you forgot an argument", not to do overload resolution.

**2. `FString::Printf` formats must be literals.** UE 5.8's
`TCheckedFormatString` is `consteval`, so the format argument has to be a
string literal *at the call*. A ternary picking between two `TEXT()`s decays
to `const wchar_t*` and produces four hundred lines of template error that
name neither the file nor the mistake. Two `Printf` calls instead of one
clever argument.
"""
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"
SIM = ROOT / "Sim"

KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "static_cast"}


def strip(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def split_top(params):
    depth, parts = 0, [""]
    for c in params:
        if c in "<([":
            depth += 1
        if c in ">)]":
            depth -= 1
        if c == "," and depth == 0:
            parts.append("")
        else:
            parts[-1] += c
    return parts


def sim_declarations():
    """{name: (min args, max args)} across all of Sim/.

    **The parameter list is found by brace matching, not by a pattern that
    forbids braces.** The first cut used `[^;{]*?` for the parameters, which
    quietly excluded every function whose dials default the way this project
    defaults them -- `const PartnerDials& dials = PartnerDials{}` -- and
    that is most of the sim. The arity rule was live and had almost nothing
    in its table, so reintroducing the bug it was written for changed
    nothing. Caught by doing exactly that.
    """
    out = {}
    for path in sorted(SIM.glob("Dirtbag*.h")):
        text = strip(io.open(path, encoding="utf-8", errors="replace").read())
        for m in re.finditer(r"^[\w:<>,\s&*]*?\b(\w+)\s*\(", text, re.M):
            name = m.group(1)
            if name in KEYWORDS:
                continue
            # Walk to the matching close paren, then insist on a `;` -- a
            # declaration, not a definition and not a call.
            i, depth = m.end(), 1
            while i < len(text) and depth > 0:
                if text[i] in "([{":
                    depth += 1
                elif text[i] in ")]}":
                    depth -= 1
                i += 1
            if depth != 0:
                continue
            tail = text[i:i + 40].lstrip()
            if not tail.startswith(";"):
                continue
            params = text[m.end():i - 1].strip()
            if not params or params == "void":
                lo = hi = 0
            else:
                parts = split_top(params)
                hi = len(parts)
                lo = sum(1 for p in parts if "=" not in p)
            if name in out:
                was_lo, was_hi = out[name]
                out[name] = (min(was_lo, lo), max(was_hi, hi))
            else:
                out[name] = (lo, hi)
    return out


def args_at(text, open_paren):
    """How many top-level arguments the call starting at `open_paren` has."""
    i, depth, args, seen = open_paren, 1, 1, False
    while i < len(text) and depth > 0:
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 1:
            args += 1
        if depth == 1 and not c.isspace() and c != ",":
            seen = True
        i += 1
    return args if seen else 0


def main():
    decls = sim_declarations()
    problems = []
    calls = 0
    printfs = 0
    files = sorted(ENGINE.rglob("*.cpp")) + sorted(ENGINE.rglob("*.h"))
    for path in files:
        raw = io.open(path, encoding="utf-8", errors="replace").read()
        text = strip(raw)

        for m in re.finditer(r"dirtbag::(\w+)\s*\(", text):
            name = m.group(1)
            if name not in decls:
                continue
            calls += 1
            lo, hi = decls[name]
            n = args_at(text, m.end())
            if n < lo or n > hi:
                problems.append(
                    "%s:%d  dirtbag::%s called with %d argument%s; declared "
                    "to take %d..%d. Somebody changed the signature and did "
                    "not change this."
                    % (path.name, text[:m.start()].count("\n") + 1, name, n,
                       "" if n == 1 else "s", lo, hi))

        for m in re.finditer(r"FString::Printf\s*\(", text):
            printfs += 1
            after = text[m.end():m.end() + 400].lstrip()
            if not after.startswith('TEXT("'):
                problems.append(
                    "%s:%d  FString::Printf's format is not a literal. UE "
                    "5.8's TCheckedFormatString is consteval -- use two "
                    "Printf calls rather than a ternary format."
                    % (path.name, text[:m.start()].count("\n") + 1))

    for line in problems:
        print("  " + line)
    print("checked %d dirtbag:: call sites and %d Printf formats across %d "
          "engine files; problems: %d" % (calls, printfs, len(files),
                                          len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

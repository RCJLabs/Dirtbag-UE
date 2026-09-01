#!/usr/bin/env python3
"""Every dirtbag:: symbol an engine file uses must be reachable from its includes.

There is no Unreal compiler here, so engine C++ is written blind and Evan's
PC is the first thing to see it. A missing include costs a whole build cycle
and looks like this:

    error C2039: 'BelayText': is not a member of 'dirtbag'

which is not a missing function -- the function exists, is tested, and is
even called from another engine file. It is a header that was never
included. DirtbagSimTypes.h aggregates eighteen of the nineteen Sim headers;
Sport was the one nobody added, so the belay functions were invisible to
DirtbagGameInstance.cpp on a commit where every container check was green.

So: for every `dirtbag::Name` written in Source/, find which Sim header
declares `Name`, and check that header is reachable through the file's own
include graph. Reachability is transitive, because that is how the engine
actually gets most of them -- through DirtbagSimTypes.h.

This cannot catch a wrong *signature*, only a missing declaration. Argument
mismatches still need the real compiler.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SIM = ROOT / "Sim"
ENGINE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def declares():
    """{symbol: {header names that declare it}}"""
    out = {}
    for header in SIM.glob("*.h"):
        text = strip_comments(header.read_text())
        names = set()
        # Functions, structs, enums, and constants -- anything the engine
        # could name through the namespace.
        names |= set(re.findall(r"\b(\w+)\s*\(", text))
        names |= set(re.findall(r"\b(?:struct|enum\s+class|enum|class)\s+(\w+)", text))
        names |= set(re.findall(r"\bconstexpr\s+\w+\s+(\w+)\s*=", text))
        for n in names:
            out.setdefault(n, set()).add(header.name)
    return out


def includes_of(path, seen=None):
    """Header names reachable from this file, transitively."""
    if seen is None:
        seen = set()
    try:
        text = strip_comments(path.read_text())
    except OSError:
        return seen
    for inc in re.findall(r'#include\s+"([^"]+)"', text):
        name = Path(inc).name
        if name in seen:
            continue
        seen.add(name)
        for base in (SIM, ENGINE):
            nxt = base / name
            if nxt.exists():
                includes_of(nxt, seen)
                break
    return seen


def main():
    decl = declares()
    problems = []
    checked = 0

    for path in sorted(list(ENGINE.rglob("*.cpp")) + list(ENGINE.rglob("*.h"))):
        text = strip_comments(path.read_text())
        used = set(re.findall(r"\bdirtbag::(\w+)", text))
        if not used:
            continue
        reachable = includes_of(path) | {path.name}
        for name in sorted(used):
            homes = decl.get(name)
            if not homes:
                continue        # enum member, nested type, or not ours
            checked += 1
            if homes & reachable:
                continue
            problems.append(
                f"{path.relative_to(ROOT)}: uses `dirtbag::{name}` but never "
                f"reaches {' or '.join(sorted(homes))}. Include it, or add it "
                f"to DirtbagSimTypes.h where the rest are aggregated.")

    print(f"checked {checked} dirtbag:: uses across the engine module; "
          f"problems: {len(problems)}")
    for p in problems:
        print("  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

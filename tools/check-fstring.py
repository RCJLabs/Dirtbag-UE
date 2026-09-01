#!/usr/bin/env python3
"""FString(...) must not be handed a std::string.

FString has constructors for char pointers and none for std::string, so
`FString(dirtbag::ReadText(...))` is a guaranteed C2440 on Evan's PC and
invisible to every other check in this repo -- the include checker proves
the name is reachable, the bridge checker proves it is compiled, and
neither knows what the constructor accepts. It cost a build cycle on
2026-08-22 (the campfire tells), which is the entry fee for a checker.

The rule needs return types, not a grep: half the sim's text helpers
return `const char*`, which FString takes happily. So: collect every
dirtbag function declared to return std::string, then flag any
`FString( dirtbag::Name(...) )` whose statement does not route the call
through `.c_str()` first.
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIM = os.path.join(ROOT, "Sim")
SRC = os.path.join(ROOT, "DirtbagUE", "Source", "DirtbagUE")

# Every function the sim declares as returning std::string.
returns_string = set()
for header in glob.glob(os.path.join(SIM, "*.h")):
    text = open(header).read()
    for m in re.finditer(r"^\s*std::string\s+(\w+)\s*\(", text, re.M):
        returns_string.add(m.group(1))

problems = []
checked = 0
for path in glob.glob(os.path.join(SRC, "*.cpp")) + glob.glob(
        os.path.join(SRC, "*.h")):
    text = open(path).read()
    for m in re.finditer(r"FString\s*\(\s*dirtbag::(\w+)\s*\(", text):
        checked += 1
        name = m.group(1)
        if name not in returns_string:
            continue   # returns const char* (or similar); FString takes it
        # The rest of the statement. If .c_str() appears before the
        # semicolon, the call was routed through it (possibly on the next
        # line -- several already are).
        stmt = text[m.start():text.find(";", m.start())]
        if ".c_str()" in stmt:
            continue
        line = text.count("\n", 0, m.start()) + 1
        problems.append("%s:%d: FString(dirtbag::%s(...)) -- %s returns "
                        "std::string; route it through .c_str()"
                        % (os.path.relpath(path, ROOT), line, name, name))

print("checked %d FString(dirtbag::...) constructions against %d "
      "string-returning sim functions; problems: %d"
      % (checked, len(returns_string), len(problems)))
for p in problems:
    print("  " + p)
sys.exit(1 if problems else 0)

#!/usr/bin/env python3
# Link-check without a linker.
#
# There is no Unreal toolchain in a container session, so engine-side C++
# here is written blind and the first compiler is Evan's PC. That made one
# failure mode expensive: a scripted edit that replaces a function by
# slicing between two anchors silently swallows every function in between.
# Brace-balance reads clean afterwards, leftover-reference greps come back
# empty, and the damage surfaces minutes later as LNK2019 on his machine.
#
# So: every function declared in a Dirtbag header must have a definition in
# the matching .cpp, qualified by the class that declared it. A bare-name
# match is not enough — the sim has free functions sharing names with wall
# methods (FinishAttempt, PeekOdds), and matching those would mask exactly
# the deletion this is looking for.
#
# Run before any commit touching DirtbagUE/Source/.
#
#   python3 tools/check-engine-defs.py

import glob
import io
import os
import re
import sys

SKIP = ("UPROPERTY", "UFUNCTION", "GENERATED_BODY", "DECLARE_", "DEFINE_",
        "typedef", "using", "friend", "return", "delete", "explicit operator")
DECL = re.compile(r"(~?\w+)\s*\([^()]*\)\s*(?:const\s*)?(?:override\s*)?;$")


def strip_noise(text):
    """Comments and string bodies out; brace structure preserved."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', text)
    text = re.sub(r"'(?:[^'\\\n]|\\.)*'", "''", text)
    return text


def class_bodies(text):
    """[(name, body)] for each class/struct, by real brace matching."""
    out = []
    for m in re.finditer(r"\b(?:class|struct)\s+(?:\w+_API\s+)?(\w+)"
                         r"(?:\s*:\s*[^{;]+)?\s*\{", text):
        depth, i = 1, m.end()
        while i < len(text) and depth:
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
            i += 1
        out.append((m.group(1), text[m.end():i - 1]))
    return out


def declarations(body):
    """Function declarations in a block, joined across wrapped lines."""
    names, pending = [], ""
    for line in body.split("\n"):
        t = line.strip()
        if not t:
            pending = ""
            continue
        pending = (pending + " " + t).strip() if pending else t
        if t.endswith(";"):
            if "(" in pending and not pending.startswith(SKIP) \
                    and "=" not in pending.split("(")[0]:
                m = DECL.search(pending)
                if m and not re.match(r"^[A-Z0-9_]+$", m.group(1)):
                    names.append((m.group(1), pending))
            pending = ""
        elif t.endswith(("{", "}", ":")):
            pending = ""
    return names


pairs = checked = 0
bad = []

for header in sorted(glob.glob("DirtbagUE/Source/DirtbagUE/*.h")):
    cpp = header[:-2] + ".cpp"
    if not os.path.exists(cpp):
        continue
    pairs += 1
    hs_raw = io.open(header, encoding="utf-8").read()
    hs = strip_noise(hs_raw)
    body = strip_noise(io.open(cpp, encoding="utf-8").read())

    bodies = class_bodies(hs)

    # Class methods: require the declaring class's own qualifier.
    for cls, cbody in bodies:
        for name, decl in declarations(cbody):
            if name == cls or name == "~" + cls:
                pass  # ctor/dtor still need a definition; same rule
            checked += 1
            qualified = re.search(r"\b%s\s*::\s*%s\s*\("
                                  % (re.escape(cls), re.escape(name)), body)
            inline = re.search(r"\b%s\s*\([^;{]*?\)\s*(?:const\s*)?\{"
                               % re.escape(name), cbody, re.S)
            if not qualified and not inline:
                bad.append((os.path.basename(header), cls, name, decl[:80]))

    # Free/namespace-scope functions: everything outside any class body.
    outside = hs
    for _, cbody in bodies:
        outside = outside.replace(cbody, "", 1)
    for name, decl in declarations(outside):
        checked += 1
        if re.search(r"\b%s\s*\([^;{]*?\)\s*(?:const\s*)?\{"
                     % re.escape(name), body, re.S):
            continue
        if re.search(r"::\s*%s\s*\(" % re.escape(name), body):
            continue
        bad.append((os.path.basename(header), "<free>", name, decl[:80]))

for f, c, n, d in bad:
    print("MISSING DEFINITION: %s -> %s::%s()   [%s]" % (f, c, n, d))
print("checked %d declarations across %d header/cpp pairs; missing: %d"
      % (checked, pairs, len(bad)))
sys.exit(1 if bad else 0)

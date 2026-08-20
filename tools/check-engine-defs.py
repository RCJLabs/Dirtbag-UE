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


def strip_macros(text):
    """Remove UHT macro invocations with balanced parens.

    Without this every UFUNCTION/UPROPERTY-decorated declaration merges into
    the macro line, gets skipped as a macro, and the entire Blueprint library
    goes unchecked — which is exactly the code most likely to lose a
    definition.
    """
    for macro in ("UFUNCTION", "UPROPERTY", "UCLASS", "USTRUCT", "UENUM",
                  "UINTERFACE", "UDELEGATE", "META"):
        while True:
            m = re.search(r"\b%s\s*\(" % macro, text)
            if not m:
                break
            depth, i = 1, m.end()
            while i < len(text) and depth:
                if text[i] == "(":
                    depth += 1
                elif text[i] == ")":
                    depth -= 1
                i += 1
            text = text[:m.start()] + text[i:]
    return text.replace("GENERATED_BODY()", "")


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
    hs = strip_macros(strip_noise(hs_raw))
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
    # Counted rather than merely matched, so that adding an overload without
    # its definition is caught — a bare name match is satisfied by any one of
    # the siblings and would sail straight past it.
    outside = hs
    for _, cbody in bodies:
        outside = outside.replace(cbody, "", 1)
    declared = {}
    for name, decl in declarations(outside):
        checked += 1
        declared.setdefault(name, []).append(decl)
    for name, decls in declared.items():
        defined = len(re.findall(
            r"\b%s\s*\([^;{]*?\)\s*(?:const\s*)?\{" % re.escape(name),
            body, re.S))
        defined += len(re.findall(r"::\s*%s\s*\(" % re.escape(name), body))
        if defined < len(decls):
            bad.append((os.path.basename(header), "<free>", name,
                        "%d declared, %d defined" % (len(decls), defined)))

# Second check, guarding the other build failure this file exists because of:
# UBT only compiles translation units under the module, so every Sim/*.cpp
# needs a one-line bridge in Source/ that #includes it. A sim file added
# without its bridge builds fine here and fails to link in the editor.
unbridged = []
engine_src = "DirtbagUE/Source/DirtbagUE"
bridge_text = ""
for cpp in glob.glob(os.path.join(engine_src, "*.cpp")):
    bridge_text += io.open(cpp, encoding="utf-8").read()
for sim in sorted(glob.glob("Sim/*.cpp")):
    if "/tests/" in sim:
        continue
    if os.path.basename(sim) not in bridge_text:
        unbridged.append(sim)

def check_blueprint_private(headers):
    """UHT: BlueprintReadWrite/ReadOnly on a private member is a hard error.

    This cost a build cycle. Three legacy properties were written into the
    private section of DirtbagGameInstance.h with BlueprintReadWrite, which
    compiles fine as C++ and fails at the UHT step with "BlueprintReadWrite
    should not be used on private members" - a stage nothing here reaches.

    Every other Blueprint-visible property in that file is public, so the
    convention was already there to follow; what was missing was anything
    that would say so before Evan's PC did.
    """
    problems = []
    checked = 0
    for h in headers:
        text = io.open(h, encoding="utf-8").read()
        access = "public"          # struct/UCLASS bodies vary; track as we go
        pending = None
        for n, line in enumerate(text.split("\n"), 1):
            bare = line.strip()
            if bare.startswith("private:"):
                access = "private"
            elif bare.startswith("protected:"):
                access = "protected"
            elif bare.startswith("public:"):
                access = "public"
            elif bare.startswith("class ") or bare.startswith("struct "):
                # A new type: UCLASS bodies default to private, USTRUCTs public.
                access = "private" if bare.startswith("class ") else "public"

            if bare.startswith("UPROPERTY("):
                pending = (n, bare, access)
                continue
            if pending and bare:
                num, prop, where = pending
                pending = None
                if "BlueprintRead" not in prop:
                    continue
                checked += 1
                # `protected` is fine and widely used here — UHT rejects
                # `private` only. A check that flagged both would have cried
                # wolf on twelve properties that build today, and a checker
                # nobody trusts is worse than no checker.
                if where == "private" and "AllowPrivateAccess" not in prop:
                    problems.append(
                        "%s:%d  BlueprintRead* on a private member - UHT "
                        "rejects this. Move it to the public section, or add "
                        "meta = (AllowPrivateAccess = \"true\")." %
                        (h, num))
    return checked, problems


for f, c, n, d in bad:
    print("MISSING DEFINITION: %s -> %s::%s()   [%s]" % (f, c, n, d))
for sim in unbridged:
    print("NO BRIDGE FILE: %s is never #included from %s/ - it will not be "
          "compiled into the module" % (sim, engine_src))
bpChecked, bpProblems = check_blueprint_private(
    glob.glob(os.path.join(engine_src, "*.h")))
for p in bpProblems:
    print(p)
print("checked %d declarations across %d header/cpp pairs; "
      "%d sim files bridged; %d Blueprint properties; problems: %d"
      % (checked, pairs, len(glob.glob("Sim/*.cpp")) - len(unbridged),
         bpChecked, len(bad) + len(unbridged) + len(bpProblems)))
sys.exit(1 if (bad or unbridged or bpProblems) else 0)

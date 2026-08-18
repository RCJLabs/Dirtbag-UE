#!/usr/bin/env python3
# Field-name check: catch `Day.Day` before the compiler does.
#
# Engine-side C++ here is written without a compiler, and the most ordinary
# mistake is reaching for a field on the wrong mirror struct - the day
# counter lives on FDirtbagPlayerState, not FDirtbagDayState, and one
# `Day.Day` cost a whole build cycle. That is bookkeeping, and bookkeeping
# is what a script is for.
#
# Scoping is per function body, which is the part that has to be right: the
# converters in DirtbagSimTypes.cpp all use `In` and `Out` for different
# types, so a file-wide symbol table reports hundreds of failures that are
# not real. A check that cries wolf gets ignored, so this one skips anything
# it cannot resolve confidently and only speaks when it is sure.
#
#   python3 tools/check-engine-fields.py

import glob
import io
import os
import re
import sys

SRC = "DirtbagUE/Source/DirtbagUE"
DECL = re.compile(
    r"\b(?:mutable\s+|const\s+|static\s+)*(F[A-Za-z]\w*)\s*[&*]?\s+(\w+)\s*(?:=|;|\)|,)")
# Pointers/references to our own UObjects and actors, so that the HUD's
# `Game->Player.Climber.Skin` - the exact shape a wrong field hides in - is
# resolved rather than skipped.
PTR = re.compile(
    r"\b(?:const\s+)?([AU]Dirtbag\w*)\s*[*&]\s*(\w+)\s*(?:=|;|\)|,)")


def strip_noise(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', text)
    return text


def matching_brace(text, open_idx):
    depth, i = 1, open_idx + 1
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return i - 1


def struct_fields(headers):
    """{struct: {field: type}} for every USTRUCT mirror."""
    out = {}
    for h in headers:
        text = strip_noise(io.open(h, encoding="utf-8").read())
        for m in re.finditer(r"\bstruct\s+(F\w+)\s*(?::\s*[^{]+)?\{", text):
            body = text[m.end():matching_brace(text, m.end() - 1)]
            fields = {}
            for f in re.finditer(
                    r"^[ \t]*((?:const\s+)?[A-Za-z_][\w:]*(?:\s*<[^>;]*>)?)"
                    r"\s+(\w+)\s*(?:=[^;]*)?;", body, re.M):
                ftype, fname = f.group(1).strip(), f.group(2)
                if ftype.split()[-1] in ("return", "GENERATED_BODY"):
                    continue
                fields[fname] = ftype.replace("const ", "").strip()
            out[m.group(1)] = fields
    return out


def class_members(headers, structs):
    """{class: {member: struct type}} - only members of mirror type."""
    out = {}
    for h in headers:
        text = strip_noise(io.open(h, encoding="utf-8").read())
        for m in re.finditer(r"\bclass\s+(?:\w+_API\s+)?([AU]\w+)"
                             r"(?:\s*:\s*[^{]+)?\{", text):
            body = text[m.end():matching_brace(text, m.end() - 1)]
            members = {}
            for d in DECL.finditer(body):
                if d.group(1) in structs:
                    members[d.group(2)] = d.group(1)
            out[m.group(1)] = members
    return out


def main():
    headers = sorted(glob.glob(os.path.join(SRC, "*.h")))
    structs = struct_fields(headers)
    members = class_members(headers, structs)
    if not structs:
        print("no USTRUCT mirrors found - wrong directory?")
        return 1

    problems, checked = [], 0
    for cpp in sorted(glob.glob(os.path.join(SRC, "*.cpp"))):
        raw = io.open(cpp, encoding="utf-8").read()
        text = strip_noise(raw)

        # Walk top-level function definitions: "Ret Class::Name(args) {...}".
        for fn in re.finditer(
                r"^[A-Za-z_][\w:<>,\s&*]*?\b(?:(\w+)::)?(\w+)\s*"
                r"\(([^;{}]*)\)\s*(?:const\s*)?\{", text, re.M):
            cls, args = fn.group(1), fn.group(3)
            body = text[fn.end():matching_brace(text, fn.end() - 1)]

            scope = {}
            scope.update(members.get(cls, {}))       # this class's members
            for d in DECL.finditer(args):            # parameters
                if d.group(1) in structs:
                    scope[d.group(2)] = d.group(1)
            for d in DECL.finditer(body):            # locals
                if d.group(1) in structs:
                    scope[d.group(2)] = d.group(1)
            ptrs = {}
            for d in PTR.finditer(args):
                ptrs[d.group(2)] = d.group(1)
            for d in PTR.finditer(body):
                ptrs[d.group(2)] = d.group(1)
            if not scope and not ptrs:
                continue

            for m in re.finditer(
                    r"(?<![\w.])(\w+)\s*(->|\.)\s*(\w+)((?:\.\w+)*)", body):
                base, sep, first, rest = m.groups()
                if sep == "->":
                    # `Game->Player.X`: resolve the first hop through the
                    # pointed-to class's members, then walk as usual.
                    if base not in ptrs or first not in members.get(
                            ptrs[base], {}):
                        continue
                    cur = members[ptrs[base]][first]
                    chain = rest
                    path = base + "->" + first
                else:
                    if base not in scope:
                        continue
                    cur = scope[base]
                    chain = "." + first + rest
                    path = base
                pos = m.start() + len(base)
                if sep == "->":
                    pos += 2 + len(first)
                for field in chain.lstrip(".").split("."):
                    if not field:
                        break
                    if cur not in structs:
                        break            # left the mirrors (TArray, FString)
                    pos += 1 + len(field)
                    if body[pos:pos + 1] == "(":
                        break            # method call, not a field
                    checked += 1
                    if field not in structs[cur]:
                        line = raw[:0].count("\n") + text[:fn.end()].count(
                            "\n") + body[:m.start()].count("\n") + 1
                        problems.append((os.path.basename(cpp), line, path,
                                         cur, field, sorted(structs[cur])))
                        break
                    path += "." + field
                    cur = structs[cur][field]

    for f, line, base, stype, field, avail in problems:
        print("NO SUCH FIELD: %s (~line %d)  '%s' is %s, which has no '%s'"
              % (f, line, base, stype, field))
        print("               %s has: %s" % (stype, ", ".join(avail)))
    print("checked %d field accesses against %d mirror structs; problems: %d"
          % (checked, len(structs), len(problems)))
    return 1 if problems else 0


sys.exit(main())

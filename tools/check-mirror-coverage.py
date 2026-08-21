#!/usr/bin/env python3
# Mirror-coverage check: catch the field the converter forgot.
#
# The other checkers ask whether the fields we DO touch exist. This one asks
# the opposite question, and it is the one that actually bit: `Weather` gained
# a `day` member, `FDirtbagWeather` did not, and `ToSim` therefore handed the
# sim a default-constructed day. Nothing failed to compile. Every window in
# the game would have been searched against a midwinter day, all year, and the
# first sign of it would have been Evan wondering why summer evenings were
# dark.
#
# So: for every `FromSim(const dirtbag::X& In)` in the converter, every field
# of the sim struct X must be read somewhere in that body. A field that is
# deliberately not surfaced gets an explicit
#
#   // mirror-skip: fieldName  -- why
#
# line inside the converter body, which is cheap to write and leaves the
# reason next to the omission.
#
#   python3 tools/check-mirror-coverage.py

import glob
import io
import re
import sys

SIM = "Sim"
CONV = "DirtbagUE/Source/DirtbagUE/DirtbagSimTypes.cpp"


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def matching_brace(text, open_idx):
    depth, i = 1, open_idx + 1
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return i - 1


def sim_structs():
    """{StructName: [field, ...]} for the plain structs in Sim/*.h."""
    out = {}
    for h in sorted(glob.glob(SIM + "/*.h")):
        text = strip_comments(io.open(h, encoding="utf-8").read())
        for m in re.finditer(r"\bstruct\s+(\w+)\s*\{", text):
            body = text[m.end():matching_brace(text, m.end() - 1)]
            # Members only: skip nested types and anything with parentheses,
            # which is a method rather than a field.
            fields = []
            for f in re.finditer(
                    r"^[ \t]*(?:const\s+)?[A-Za-z_][\w:]*(?:\s*<[^>;]*>)?"
                    r"\s+(\w+)\s*(?:\[[^\]]*\])?\s*(?:=[^;]*)?;",
                    body, re.M):
                name = f.group(1)
                if name not in fields:
                    fields.append(name)
            out[m.group(1)] = fields
    return out


def converters():
    """[(SimType, body, line)] for each FromSim in the converter file."""
    text = io.open(CONV, encoding="utf-8").read()
    out = []
    for m in re.finditer(
            r"F\w+\s+FromSim\s*\(\s*const\s+dirtbag::(\w+)\s*&\s*(\w+)\s*\)\s*\{",
            text):
        body = text[m.end():matching_brace(text, m.end() - 1)]
        line = text[:m.start()].count("\n") + 1
        out.append((m.group(1), m.group(2), body, line))
    return out


def to_sim_converters():
    """[(SimType, out_var, body, line)] for each ToSim in the converter file.

    The mirrored question, and the one that was going unasked: FromSim is
    checked for every sim field it *reads*, and ToSim was checked for
    nothing at all. Deleting one line of a ToSim -- the engine-to-sim half
    of a field -- passed every checker and every test in the repo, while
    throwing away a value the sim writes every night.
    """
    text = io.open(CONV, encoding="utf-8").read()
    out = []
    for m in re.finditer(
            r"dirtbag::(\w+)\s+ToSim\s*\(\s*const\s+F\w+\s*&\s*\w+\s*\)\s*\{",
            text):
        body = text[m.end():matching_brace(text, m.end() - 1)]
        line = text[:m.start()].count("\n") + 1
        # The local being filled in, found by its declaration rather than
        # assumed to be called Out.
        decl = re.search(r"dirtbag::" + re.escape(m.group(1)) + r"\s+(\w+)\s*;",
                         body)
        out.append((m.group(1), decl.group(1) if decl else "Out", body, line))
    return out


def main():
    structs = sim_structs()
    problems = []
    checked = 0

    for sim_type, param, body, line in converters():
        if sim_type not in structs:
            continue
        skipped = set(re.findall(r"//\s*mirror-skip:\s*(\w+)", body))
        read = set(re.findall(r"\b" + re.escape(param) + r"\.(\w+)", body))
        for field in structs[sim_type]:
            checked += 1
            if field in read or field in skipped:
                continue
            problems.append(
                "%s:%d  FromSim(dirtbag::%s) never reads `%s.%s` - the mirror "
                "drops it. Carry it, or say why with `// mirror-skip: %s`."
                % (CONV, line, sim_type, param, field, field))

    for sim_type, out_var, body, line in to_sim_converters():
        if sim_type not in structs:
            continue
        skipped = set(re.findall(r"//\s*mirror-skip:\s*(\w+)", body))
        written = set(re.findall(
            r"\b" + re.escape(out_var) + r"\.(\w+)\s*(?:=|\.|\[)", body))
        # A field filled by a helper -- push_back, resize, assign -- counts.
        written |= set(re.findall(
            r"\b" + re.escape(out_var) + r"\.(\w+)\s*\.", body))
        for field in structs[sim_type]:
            checked += 1
            if field in written or field in skipped:
                continue
            problems.append(
                "%s:%d  ToSim(-> dirtbag::%s) never fills `%s.%s` - the sim "
                "gets a default. Fill it, or say why with "
                "`// mirror-skip: %s`."
                % (CONV, line, sim_type, out_var, field, field))

    for p in problems:
        print("  " + p)
    print("checked %d sim fields across %d converters; problems: %d"
          % (checked, len(converters()) + len(to_sim_converters()),
             len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

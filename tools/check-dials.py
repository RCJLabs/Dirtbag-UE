#!/usr/bin/env python3
"""Every dial must be read by something, and every mirrored dial pinned.

The project's first hard constraint is "dials over magic numbers". This
checks the constraint has not quietly inverted into magic numbers wearing a
dial's clothes:

  1. A dial nothing reads. It advertises a tuning that does not exist —
     somebody moves it, nothing happens, and the only symptom is that the
     game does not respond to its own tuning surface. Found three:
     AgeDials::techniquePeak and ::headPeak (sentinels for a code path that
     is deliberately absent) and KitDials::groundedFraction.

  2. A dial name that appears in more than one Dials struct. That is this
     project's deliberate arrangement for a value two systems both need --
     share the function, mirror the constant -- and there are four of them.
     It is only safe while something pins the copies together, so each pair
     must be named in TestTheMirroredDialsStillAgree. KitDials' two pad
     numbers were mirrored, unpinned, and read by nothing; the pad could be
     retuned at the shop with no effect at the wall, silently.

Rule 1 has a subtlety worth keeping: a naive search for `.fieldName` cannot
tell KitDials::noPadGradePenalty from SessionDials::noPadGradePenalty, so a
dead dial hides behind its own mirror. Mirrored names are therefore checked
by rule 2 rather than rule 1, which is the only honest way to split them
without resolving C++ types.

Opt out with `// dial-unread-ok: <why>` above the field.
"""
import glob
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PINS = ROOT / "Sim" / "tests" / "test_main.cpp"
PIN_TEST = "TestTheMirroredDialsStillAgree"


def strip_block(text):
    return re.sub(r"/\*.*?\*/", "", text, flags=re.S)


def matching_brace(text, open_idx):
    depth, i = 1, open_idx + 1
    while i < len(text) and depth:
        depth += (text[i] == "{") - (text[i] == "}")
        i += 1
    return i - 1


def dial_fields():
    """{(StructName, field): (header, opted_out_reason_or_None)}"""
    out = {}
    for header in sorted(glob.glob(str(ROOT / "Sim" / "*.h"))):
        raw = strip_block(io.open(header, encoding="utf-8").read())
        for m in re.finditer(r"\bstruct\s+(\w*Dials)\s*\{", raw):
            body = raw[m.end():matching_brace(raw, m.end() - 1)]
            lines = body.splitlines()
            for i, line in enumerate(lines):
                clean = re.sub(r"//.*", "", line)
                f = re.match(
                    r"^[ \t]*(?:const\s+)?[A-Za-z_][\w:]*\s+(\w+)\s*"
                    r"(?:=[^;]*)?;", clean)
                if not f:
                    continue
                reason = None
                for back in range(i - 1, max(-1, i - 10), -1):
                    prev = lines[back].strip()
                    if not prev.startswith("//"):
                        break
                    hit = re.search(r"dial-unread-ok:\s*(.+)", prev)
                    if hit:
                        reason = hit.group(1).strip()
                        break
                out[(m.group(1), f.group(1))] = (Path(header).name, reason)
    return out


def main():
    fields = dial_fields()

    hay = ""
    for pat in ("Sim/*.cpp", "Sim/*.h", "Sim/tools/*.cpp"):
        for p in glob.glob(str(ROOT / pat)):
            hay += io.open(p, errors="ignore", encoding="utf-8").read()
    for p in (ROOT / "DirtbagUE" / "Source" / "DirtbagUE").rglob("*"):
        if p.suffix in (".cpp", ".h"):
            hay += io.open(p, errors="ignore", encoding="utf-8").read()

    pins = io.open(PINS, encoding="utf-8").read()
    body = ""
    at = pins.find("static void " + PIN_TEST)
    if at >= 0:
        body = pins[at:matching_brace(pins, pins.index("{", at))]

    by_name = {}
    for (struct, name) in fields:
        by_name.setdefault(name, []).append(struct)

    problems = []
    mirrored = 0
    for (struct, name), (header, reason) in sorted(fields.items()):
        if reason:
            continue
        if len(by_name[name]) > 1:
            # Rule 2: a mirror. Reads cannot be attributed by name, so the
            # requirement is that the test pins the pair.
            mirrored += 1
            if name not in body:
                problems.append(
                    f"{header}: {struct}::{name} is mirrored across "
                    f"{', '.join(sorted(by_name[name]))} but {PIN_TEST} "
                    f"does not pin it. Two copies of one number drift, and "
                    f"the drift is silent.")
            continue
        if not re.search(r"\.\s*" + re.escape(name) + r"\b", hay):
            problems.append(
                f"{header}: {struct}::{name} is read by nothing. Wire it, "
                f"delete it, or say why with `// dial-unread-ok: <reason>`.")

    print(f"checked {len(fields)} dials across "
          f"{len({s for s, _ in fields})} structs "
          f"({mirrored} mirrored and pinned); problems: {len(problems)}")
    for p in problems:
        print("  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

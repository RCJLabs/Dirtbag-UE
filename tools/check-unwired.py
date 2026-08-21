#!/usr/bin/env python3
"""Every public sim function must have a caller in the live game.

Nine times now a function has been written, tested, and called by nothing:
KitDay (one $75 bought 365 days of membership), the body's injury roll,
FactionDay (a shut crag would have stayed shut forever), the legacy save
path, ethics discovery, EthicsNews (set nightly and drawn nowhere),
head training (a whole skill axis, read in four places and written in
none), CreditFirstAscent (every first ascent was worth no opinion to any
faction), and CoversShoes (the bottom sponsorship rung gave you nothing).

Every one of them compiled. Every one of them had passing tests. Tests
prove a function does what it says; only this proves anybody asked.

"Live" means *reachable from the engine module*, transitively. The season
probe deliberately does not count, and that is the whole point: FactionDay
was called by the probe every day of a simulated season and by the engine
never, so a shut crag would have stayed shut forever in the actual game
while every measurement said the mechanic worked. A harness calling
something is not the same as the game calling it.

So: functions named anywhere under Source/ are roots, and anything a live
function calls is live too -- including through its own file, which is how
CreditFirstAscent is wired now that ClaimFirstAscent calls it.

Opt out in the header, on the line above the declaration:

    // unwired-ok: nothing calls this yet -- it is the guidebook page's
    // formatter and there is no guidebook screen.

The reason is not decoration. Every entry below was once a real bug.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SIM = ROOT / "Sim"
ENGINE = ROOT / "DirtbagUE" / "Source" / "DirtbagUE"
PROBE = SIM / "tools" / "season.cpp"

KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "catch"}
DECL = re.compile(r"^[A-Za-z_][\w:<>,\s\*&]*?\b(\w+)\s*\(")


def strip_block_comments(text):
    return re.sub(r"/\*.*?\*/", "", text, flags=re.S)


def declarations(header):
    """(name, line_number, opted_out_reason_or_None) for each declaration.

    Declarations wrap across lines as often as definitions do, and reading
    only the first line quietly skips every one that does -- which is a
    checker under-reporting, the one failure mode worse than not having it.
    """
    out = []
    lines = strip_block_comments(header.read_text()).splitlines()
    clean = [re.sub(r"//.*", "", ln) for ln in lines]
    i = 0
    while i < len(clean):
        line = clean[i].strip()
        if not line or line.startswith("#"):
            i += 1
            continue
        m = DECL.match(line)
        if not m or m.group(1) in KEYWORDS:
            i += 1
            continue
        # Accumulate until the signature's parens balance, then look at
        # what comes next: `;` is a declaration, `{` is a definition.
        # Testing for a brace anywhere is wrong -- half these declarations
        # carry `= VanDials{}` default arguments, and reading that as a
        # definition silently drops them, which is how the first cut of this
        # checker lost thirty of them without saying a word.
        j, sig, depth = i, "", 0
        while j < len(clean):
            sig += clean[j]
            depth += clean[j].count("(") - clean[j].count(")")
            j += 1
            if depth <= 0 and "(" in sig:
                break
        tail = sig[sig.rindex(")") + 1:].strip() if ")" in sig else ""
        if not tail.startswith(";"):
            i = max(j, i + 1)
            continue
        # An opt-out is a `// unwired-ok:` comment anywhere in the comment
        # block immediately above, so it can be written as prose.
        reason = None
        for back in range(i - 1, max(-1, i - 14), -1):
            prev = lines[back].strip()
            if not prev.startswith("//"):
                break
            hit = re.search(r"unwired-ok:\s*(.+)", prev)
            if hit:
                reason = hit.group(1).strip()
                break
        out.append((m.group(1), i + 1, reason))
        i = max(j, i + 1)
    return out


def bodies(text):
    """Top-level function bodies in a .cpp, as {name: [body, ...]}.

    Signatures wrap, and one-liners exist, so this accumulates lines until
    it finds the `{` or the `;` rather than trusting either to land on the
    first line. Getting that wrong silently drops definitions, and a
    definition this checker cannot see is a function it calls unwired.
    """
    out = {}
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = DECL.match(lines[i])
        if not m or m.group(1) in KEYWORDS:
            i += 1
            continue
        # Gather the signature until the brace or the semicolon.
        j = i
        sig = ""
        while j < len(lines) and "{" not in sig and ";" not in sig:
            sig += lines[j]
            j += 1
        if "{" not in sig or (";" in sig and sig.index(";") < sig.index("{")):
            i += 1                       # a declaration, not a definition
            continue
        depth = sig.count("{") - sig.count("}")
        body = [sig[sig.index("{") + 1:]]
        while j < len(lines) and depth > 0:
            depth += lines[j].count("{") - lines[j].count("}")
            body.append(lines[j])
            j += 1
        out.setdefault(m.group(1), []).append("\n".join(body))
        i = max(j, i + 1)
    return out


def called_in(text):
    return set(re.findall(r"\b(\w+)\s*\(", text))


def main():
    sim_text = {p: strip_block_comments(p.read_text()) for p in SIM.glob("*.cpp")}

    # Every top-level definition in the sim, and what it calls.
    calls = {}
    for text in sim_text.values():
        for name, blocks in bodies(text).items():
            for block in blocks:
                calls.setdefault(name, set()).update(called_in(block))

    # A default argument is a call site too: LoadSave(..., migrations =
    # DefaultMigrations()) is the only thing that wires the migration
    # registry in, and it lives in a header rather than a body.
    for header in SIM.glob("*.h"):
        text = strip_block_comments(header.read_text())
        for m in re.finditer(r"=\s*(\w+)\s*\(", text):
            calls.setdefault("<default-arguments>", set()).add(m.group(1))

    # Roots: anything the engine module names. The probe is not a root.
    engine = ""
    for path in list(ENGINE.rglob("*.cpp")) + list(ENGINE.rglob("*.h")):
        engine += path.read_text()
    live = {n for n in calls if re.search(r"\b" + re.escape(n) + r"\s*\(", engine)}
    # Default arguments belong to whichever function carries them, and every
    # such function is declared in the same headers, so treat them as live
    # roots rather than trying to attribute each one.
    live.add("<default-arguments>")

    # Anything a live function calls is live too.
    frontier = list(live)
    while frontier:
        n = frontier.pop()
        for callee in calls.get(n, ()):
            # Not gated on having found the callee's own definition: the
            # small inline helpers -- Clamp01 and friends -- live entirely
            # in headers, and requiring a .cpp body for them would report
            # the most-used functions in the sim as the least-used.
            if callee not in live:
                live.add(callee)
                frontier.append(callee)

    problems = []
    owed = []
    checked = exempted = 0
    for header in sorted(SIM.glob("*.h")):
        for name, line, reason in declarations(header):
            checked += 1
            if reason:
                exempted += 1
                if reason.startswith("NOT WIRED"):
                    owed.append((name, reason))
                continue
            if name in live:
                continue
            # A declaration with no definition anywhere is a header-only
            # inline or a template; nothing to say about it.
            if name not in calls and not re.search(
                    r"\b" + re.escape(name) + r"\s*\(", engine):
                if not any(re.search(r"\b" + re.escape(name) + r"\s*\(", t)
                           for t in sim_text.values()):
                    continue
            problems.append(
                f"{header.relative_to(ROOT)}:{line}: {name}() is not reachable "
                f"from the engine.\n"
                f"    Wire it into the game, or say why not with a "
                f"`// unwired-ok: <reason>` comment above the declaration."
            )

    # An opt-out whose reason starts NOT WIRED is debt, not a decision. It
    # is counted separately and loudly so that "0 problems" can never come
    # to mean "nothing to do" — the whole failure mode this checker exists
    # to end is a mechanic that looks finished because nothing complains.
    debt = sorted(n for n, r in owed)
    print(f"checked {checked} public sim declarations "
          f"({exempted} explicitly unwired); problems: {len(problems)}")
    if debt:
        print(f"  {len(debt)} of those are NOT WIRED and tracked as debt: "
              + ", ".join(debt))
    for p in problems:
        print("  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Text replacement that fails loudly when it does not apply.

Twice in one session a scripted edit here quietly matched nothing: an
anchor's whitespace or comment had drifted, `str.replace` returned the file
unchanged, and the edit vanished without a word. Once it removed a
declaration while leaving its use behind (an unresolved external on Evan's
PC); once it left a property undeclared while the .cpp already referenced it.

There is no Unreal compiler here to catch either, so the replacement itself
has to be the thing that complains.

    from patch import edit
    edit(path, old, new)          # exactly one occurrence, or it raises
    edit(path, old, new, count=3) # exactly three
"""

import io


def edit(path, old, new, count=1):
    text = io.open(path, encoding="utf-8").read()
    found = text.count(old)
    if found != count:
        raise SystemExit(
            "PATCH FAILED in %s\n"
            "  expected %d occurrence(s), found %d\n"
            "  anchor: %r" % (path, count, found, old[:160]))
    io.open(path, "w", encoding="utf-8").write(text.replace(old, new, count))
    return found


def insert_before(path, anchor, text):
    return edit(path, anchor, text + anchor)


def insert_after(path, anchor, text):
    return edit(path, anchor, anchor + text)


def edits(*specs):
    """Apply several edits, and fail only after checking all of them.

    `edit` raises SystemExit on a bad anchor, which kills the whole script —
    so every edit written after the failing one silently never runs. That has
    now happened three times in this project, each time producing a file that
    looked edited, compiled fine, and was missing half the change: a printf
    that gained values but not format specifiers, a migration registered
    nowhere, a save that wrote a field it never read back.

    This checks every anchor first and applies nothing unless all of them
    match, so a partial application is not a state the repo can reach. Each
    spec is (path, old, new) or (path, old, new, count).

        edits(("a.cpp", "x", "y"),
              ("a.cpp", "p", "q"))
    """
    import io as _io

    # Anchors are checked against the file as it will be when that edit runs,
    # so two edits to the same file compose the way they read.
    pending = {}
    problems = []
    for i, spec in enumerate(specs):
        path, old, new = spec[0], spec[1], spec[2]
        count = spec[3] if len(spec) > 3 else 1
        if path not in pending:
            pending[path] = _io.open(path, encoding="utf-8").read()
        found = pending[path].count(old)
        if found != count:
            problems.append(
                "  edit %d of %d in %s: expected %d, found %d\n    anchor: %r"
                % (i + 1, len(specs), path, count, found, old[:120]))
            continue
        pending[path] = pending[path].replace(old, new, count)

    if problems:
        raise SystemExit("PATCH FAILED — nothing was written.\n" +
                         "\n".join(problems))

    for path, text in pending.items():
        _io.open(path, "w", encoding="utf-8").write(text)
    return len(specs)

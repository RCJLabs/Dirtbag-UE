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

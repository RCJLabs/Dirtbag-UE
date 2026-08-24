#!/usr/bin/env python3
"""What has never once happened.

Nearly every real bug this project has found late was **a zero in a column
nobody read**:

  nobelayer  0    the belayer gate had never bitten in ninety years
  starved    0    "sleeping hungry ruins the night" had never fired, ever
  gaway      0    nobody was ever away, because nobody ever left town
  sport      0    no career had ever climbed at the Cave

Each was visible in the probe's own output for weeks. Each took a person
noticing. This asks the question on purpose instead.

Method: run `build/season` across a battery of policies and seeds, and
report every counter that came out **zero in every single run**. A column
that is zero for one policy is fine -- a boulderer never needs a belayer.
A column that is zero for *all* of them is a rule the game has, and never
applies.

    python3 tools/never-happened.py            # the standard battery
    python3 tools/never-happened.py --seeds 6  # slower, fewer false alarms

**Run it at a full career.** The default is thirty years because that is
what this game is, and the first run of this tool at ten reported the whole
top of the competition ladder dead -- the national team, the World Cup, the
Games. It is not dead; it needs a ranking of 700 and a career gets there in
its twenties. A shorter run is faster and its output is a list of things
that have not happened *yet*, which is a different question and a worse
one.

Not part of preflight: it takes minutes rather than seconds, and its
output is a reading list rather than a pass/fail. Run it after a phase
lands, which is when a system most likely arrived with nothing reaching
it.
"""

import argparse
import subprocess
import sys

# The battery. Chosen to span the game's shapes rather than to be
# exhaustive -- somebody who boulders, somebody on a rope, somebody who
# leads, somebody who competes, somebody with a life, somebody who never
# works -- **and the option knobs, which is the half the first version of
# this tool got wrong.**
#
# Run with policies alone it reported twenty-seven dead counters, and most
# of them were dead because nothing in the battery had a rival, saw a
# physio or entered a race. A coverage tool whose gaps are its own is worse
# than no coverage tool, because its output reads like a bug list.
RUNS = [
    ("greedy", []),
    ("sporty", []),
    ("trad", []),
    ("comper", []),
    ("lifer", []),
    ("kept", []),
    ("projector", []),
    ("sponsored", []),
    ("kitted", []),
    ("dreamer", []),
    ("racer", []),
    ("stakeout", []),
    ("hoarder", []),
    ("saver", []),
    ("careful", []),
    ("salary", []),
    # Somebody to beat, which nothing above has.
    ("greedy", ["rival=1"]),
    ("comper", ["rival=1"]),
    # And the two ways of handling being hurt, which is the only route to
    # every column in the medical file.
    ("greedy", ["med=sensible"]),
    ("greedy", ["med=impatient"]),
    ("greedy", ["med=careful-ins"]),
    # A dynasty, which is the only way anybody retires.
    ("greedy", ["careers=1", "retire=late"]),
    # And the bold season, where head is trained and nothing is padded.
    ("greedy", ["pads=0"]),
]

SEEDS = ["crag-1", "valley-7", "north-3", "east-11", "west-5", "granite-2"]

# Columns where zero is the answer rather than a hole, with the reason.
# Anything not listed here is expected to happen to somebody, eventually.
EXPECTED_ZERO = {
    "rest": "an argument echo, not a counter",
    "skinregen": "an argument echo, not a counter",
    "firstquirk": "-1 means never, and is reported as its own value",
    "rackday": "-1 means never",
    "met": "-1 means never",
    "lost": "-1 means never",
    "firstgreet": "-1 means never",
}


def run(days, seed, policy, extra):
    out = subprocess.run(["./build/season", str(days), seed, "3.0", "q", policy]
                         + extra, capture_output=True, text=True).stdout
    head = row = None
    for line in out.splitlines():
        if line.startswith("HEAD\t"):
            head = line.split("\t")[1:]
        elif line.startswith("ROW\t"):
            row = line.split("\t")[1:]
    if not head or not row:
        return {}
    return dict(zip(head, row))


def is_zero(value):
    try:
        return float(value) == 0.0
    except ValueError:
        return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--days", type=int, default=10950)
    ap.add_argument("--seeds", type=int, default=len(SEEDS))
    ap.add_argument("--extra", nargs="*", default=[])
    args = ap.parse_args()

    seeds = SEEDS[:args.seeds]
    runs = 0
    # A column is "live" the moment any run gives it a non-zero.
    live = set()
    seen = []
    for policy, extra in RUNS:
        for seed in seeds:
            row = run(args.days, seed, policy, extra + args.extra)
            if not row:
                print("could not run %s %s -- is build/season built?"
                      % (policy, " ".join(extra)))
                return 2
            runs += 1
            if not seen:
                seen = list(row.keys())
            for key, value in row.items():
                if not is_zero(value):
                    live.add(key)
        print("  ran %-10s %-22s x %d seeds"
              % (policy, " ".join(extra), len(seeds)), file=sys.stderr)

    never = [k for k in seen if k not in live and k not in EXPECTED_ZERO]
    known = [k for k in seen if k not in live and k in EXPECTED_ZERO]

    print()
    print("%d runs: %d configurations x %d seeds x %d days"
          % (runs, len(RUNS), len(seeds), args.days))
    print("%d counters, %d of them moved at least once."
          % (len(seen), len(live)))
    print()
    if known:
        print("zero on purpose:")
        for k in known:
            print("  %-14s %s" % (k, EXPECTED_ZERO[k]))
        print()
    if not never:
        print("NOTHING NEVER HAPPENS -- every counter moved for somebody.")
        return 0
    print("NEVER HAPPENED to anybody, in any of these careers:")
    for k in never:
        print("  %s" % k)
    print()
    print("Each of these is a rule the game has and never applies. That is")
    print("not automatically a bug -- it may want a policy that provokes it")
    print("-- but every one of them is worth a reason written down.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

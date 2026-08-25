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

**A "never happened" is a claim about the battery until you have checked
it against the probe's own flags.** This tool has now had its own coverage
be the bug three separate times -- policies without knobs, careers too
short, and a `med=` battery missing the `-up` and `-hard` suffixes whose
comments in season.cpp say in as many words that they exist so those very
counters can fire. Before believing an entry, grep season.cpp for the
counter and read what turns it on.

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
    # And the ways of handling being hurt. **All of the suffixes**, which
    # is the half this tool got wrong a third time: `med=` composes, and
    # `-up` (prehab, meds, the tooth) and `-hard` (answer every shift the
    # hard way) are separate flags whose own comments in season.cpp say
    # they exist precisely so those things are reachable. A battery with
    # `careful-ins` and without `careful-up` reports the upkeep half of
    # the medical system dead, and it is not dead -- it is unasked-for.
    ("greedy", ["med=sensible"]),
    ("greedy", ["med=impatient"]),
    ("greedy", ["med=careful-ins"]),
    ("greedy", ["med=careful-up"]),
    ("greedy", ["med=careful-up-ins"]),
    ("greedy", ["med=sensible-up"]),
    ("greedy", ["med=impatient-up"]),
    # Somebody who answers every shift the hard way, which is the only
    # route to botching, to losing standing for it, and to being sacked.
    ("greedy", ["med=-hard"]),
    ("salary", ["med=-hard"]),
    # A dynasty, which is the only way anybody retires.
    ("greedy", ["careers=1", "retire=late"]),
    # And the bold season, where head is trained and nothing is padded.
    ("greedy", ["pads=0"]),
    # **The gym, both passes.** A battery with no gym in it reports the
    # whole business dead -- the same coverage failure the medical
    # suffixes caused, and the fifth time this project has been bitten by
    # a tool whose own reach was the bug. `gym=1` is the P&L engine with
    # every lever left where it starts; `gym=run` staffs it, builds onto
    # it and answers what lands on the clipboard. It needs a wallet that
    # can reach $25,000, which is what the hoard is for.
    ("hoarder", ["savings=60000", "gym=1"]),
    ("hoarder", ["savings=60000", "gym=run"]),
    # ...and `gym=life`, which is `run` plus the people in it: walking the
    # floor and hosting comp nights. Without it the whole GYM-2/GYM-5 layer
    # reports dead, which is the same coverage failure again.
    ("hoarder", ["savings=60000", "gym=life"]),
    # ...and the same with somebody to beat, because GYM-8's payoff lands
    # one system over: a graduate takes the seat when the rival generation
    # turns over, and a career with no rival never turns one over. Without
    # this row `youthstep` reports dead and the whole point of the youth
    # team looks unreachable.
    ("hoarder", ["savings=60000", "gym=life", "rival=1"]),
    # ...and the squad handed over, which is the only way `gainHired` is
    # ever read. In the source it never is: its session verb is gated on
    # coaching them yourself and nothing runs one anywhere else, so the paid
    # coach costs $30 a day for kids who stop entirely.
    ("hoarder", ["savings=60000", "gym=hired", "rival=1"]),
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

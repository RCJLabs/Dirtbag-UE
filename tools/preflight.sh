#!/usr/bin/env bash
# Everything that can be checked without Unreal. Run before any commit that
# touches Sim/ or DirtbagUE/Source/.
#
# There is no Unreal toolchain in a container session, so engine-side C++ is
# written blind and the first compiler is Evan's PC. Each check here exists
# because a specific mistake reached that PC and cost a build cycle:
#
#   sim tests + unity   a second anonymous-namespace Clamp01; every file is
#                       its own unit here, one unit in UBT
#   engine-defs         a scripted edit that sliced out FinishAttempt and
#                       EndSession; brace-balance cannot see a deleted
#                       function, and a bare-name search for FinishAttempt
#                       is satisfied by the sim's own
#   engine-defs         a sim file with no bridge .cpp, so UBT never
#                       compiled it
#   engine-fields       `Day.Day`, when the day counter lives on
#                       FDirtbagPlayerState
#   bodycontext         the comp resolver assembled its own AttemptInput and
#                       set six fields, so a climber with a wrecked finger,
#                       a flu and an abscess scored the same as a fresh one
#                       at a comp -- in the path that serves comps, the
#                       circuit, the World Cup, the Games and league nights
set -euo pipefail
cd "$(dirname "$0")/.."

echo "== sim harness + unity build =="
./Sim/run-tests.sh

# **The probe is a build too, and nothing here was building it.**
# `SpendDayWith` gained a required argument in the turnout commit and
# season.cpp was never updated, so every measurement claim between that
# commit and this one could not have been re-run -- and preflight stayed
# green throughout, because check-parity reads the probe's source and
# never-happened.py runs a binary that was already on disk. A tool's own
# coverage is the bug, for the fourth time.
echo
echo "== the probe still builds =="
./Sim/tools/build-season.sh >/dev/null
echo "OK  build/season is current"

echo
echo "== engine definitions and bridge files =="
python3 tools/check-engine-defs.py

echo
echo "== engine field names =="
python3 tools/check-engine-fields.py

echo
echo "== mirror coverage =="
python3 tools/check-mirror-coverage.py

echo
echo "== sim reachable from the engine =="
python3 tools/check-unwired.py

echo
echo "== dials read, and mirrors pinned =="
python3 tools/check-dials.py

echo
echo "== engine includes reach what the engine names =="
python3 tools/check-engine-includes.py

echo
echo "== FString never fed a std::string =="
python3 tools/check-fstring.py

echo
echo "== log categories defined once =="
python3 tools/check-logcat.py

echo
echo "== asset slots are actually played =="
python3 tools/check-cues.py

echo
echo "== every attempt is climbed by a body =="
python3 tools/check-bodycontext.py

echo
echo "== every gameplay verb has a door =="
python3 tools/check-doors.py

echo
echo "== the measured game is the played game =="
python3 tools/check-parity.py

echo
echo "ALL CHECKS PASSED"

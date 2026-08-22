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
set -euo pipefail
cd "$(dirname "$0")/.."

echo "== sim harness + unity build =="
./Sim/run-tests.sh

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
echo "ALL CHECKS PASSED"

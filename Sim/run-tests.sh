#!/usr/bin/env bash
# Build and run the Dirtbag sim core's standalone test harness with plain g++.
# The same .cpp files compile inside the Unreal module — this harness is what
# keeps the sim honest before (and after) the editor exists.
set -euo pipefail
cd "$(dirname "$0")"

mkdir -p ../build
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
    DirtbagRng.cpp DirtbagCore.cpp DirtbagSession.cpp tests/test_main.cpp \
    -o ../build/sim_tests

../build/sim_tests

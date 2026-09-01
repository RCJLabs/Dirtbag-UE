#!/usr/bin/env bash
# Build and run the season probe. Not part of run-tests.sh: it explores
# rather than asserts, and its job is to find the things a test does not
# know to ask about yet.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p ../build
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. \
    DirtbagRng.cpp DirtbagCore.cpp DirtbagSession.cpp DirtbagSessionLoop.cpp \
    DirtbagDay.cpp DirtbagSave.cpp DirtbagConditions.cpp DirtbagCrag.cpp \
    DirtbagFirstAscent.cpp DirtbagPartner.cpp DirtbagDog.cpp DirtbagGear.cpp DirtbagVan.cpp DirtbagJobs.cpp DirtbagFactions.cpp DirtbagTown.cpp DirtbagKit.cpp DirtbagBody.cpp DirtbagAge.cpp DirtbagSport.cpp DirtbagTrad.cpp DirtbagHabits.cpp DirtbagNarrator.cpp DirtbagLife.cpp DirtbagLocals.cpp DirtbagGym.cpp DirtbagGymTown.cpp DirtbagGymFloor.cpp DirtbagGymLeague.cpp DirtbagSpeed.cpp DirtbagTax.cpp DirtbagYouth.cpp DirtbagLiving.cpp DirtbagBivy.cpp DirtbagLegacy.cpp DirtbagSponsor.cpp DirtbagEthics.cpp DirtbagCharacter.cpp DirtbagRival.cpp DirtbagComp.cpp DirtbagTeam.cpp DirtbagWorldStage.cpp DirtbagLeague.cpp DirtbagMedical.cpp DirtbagAilments.cpp DirtbagBodyContext.cpp DirtbagCraft.cpp DirtbagCampfire.cpp DirtbagCrew.cpp DirtbagDreams.cpp DirtbagZones.cpp \
    tools/season.cpp -o ../build/season
../build/season "$@"

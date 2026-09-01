// Copyright Epic Games, Inc. All Rights Reserved.

#include "DirtbagUE.h"
#include "Modules/ModuleManager.h"

#include "DirtbagGameInstance.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, DirtbagUE, "DirtbagUE" );

DEFINE_LOG_CATEGORY(LogDirtbagUE)

// Declared in DirtbagGameInstance.h; defined once, here, so that every
// actor can report a setup mistake without any two of them colliding in a
// unity build.
DEFINE_LOG_CATEGORY(LogDirtbagSetup);
// Copyright Epic Games, Inc. All Rights Reserved.

#include "DirtbagUEGameMode.h"

#include "DirtbagRng.h"

DEFINE_LOG_CATEGORY_STATIC(LogDirtbagSim, Log, All);

ADirtbagUEGameMode::ADirtbagUEGameMode()
{
	// stub
}

void ADirtbagUEGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Must match the frozen vector in Sim/tests/test_main.cpp digit for
	// digit; a mismatch means the sim is not the sim, and nothing gets
	// built until it is.
	dirtbag::Rng Rng = dirtbag::Rng::FromSeed("golden");
	for (int i = 0; i < 5; i++)
	{
		UE_LOG(LogDirtbagSim, Display, TEXT("golden[%d] = %.17g"), i, Rng.NextDouble());
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DirtbagUEGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ADirtbagUEGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ADirtbagUEGameMode();

protected:

	/** Logs the sim core's golden RNG vector on startup (SETUP.md §2) —
	 *  proof the port is bit-exact before anything is built on it. */
	virtual void BeginPlay() override;

};




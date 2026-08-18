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

	/** Draws the ambient day readout — day, clock, cash, body — so the life
	 *  sim is legible while you walk around, not only mid-session. */
	virtual void Tick(float DeltaSeconds) override;
};




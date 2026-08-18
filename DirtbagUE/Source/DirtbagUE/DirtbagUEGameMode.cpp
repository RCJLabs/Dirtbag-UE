// Copyright Epic Games, Inc. All Rights Reserved.

#include "DirtbagUEGameMode.h"

#include "Engine/Engine.h"

#include "DirtbagGameInstance.h"
#include "DirtbagRng.h"

DEFINE_LOG_CATEGORY_STATIC(LogDirtbagSim, Log, All);

ADirtbagUEGameMode::ADirtbagUEGameMode()
{
	// AInfo (via AGameModeBase) disables ticking two ways; the ambient HUD
	// needs both flags flipped or it silently never draws.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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

void ADirtbagUEGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UDirtbagGameInstance* Game = Cast<UDirtbagGameInstance>(GetGameInstance());
	if (!Game || !GEngine)
	{
		return;
	}

	const int32 Hour =
	    FMath::Clamp(FMath::FloorToInt(static_cast<float>(Game->Day.Hour)), 0, 23);
	const int32 Minute = FMath::Clamp(
	    FMath::FloorToInt(static_cast<float>((Game->Day.Hour - Hour) * 60.0)), 0,
	    59);

	// Keyed rows replace themselves in place rather than scrolling.
	GEngine->AddOnScreenDebugMessage(
	    1, 0.5f, FColor::White,
	    FString::Printf(
	        TEXT("DAY %d   %02d:%02d   $%.0f   energy %.0f   hunger %.0f   skin %.1f"),
	        Game->Player.Day, Hour, Minute, Game->Player.Cash, Game->Day.Energy,
	        Game->Day.Hunger, Game->Player.Climber.Skin));
	GEngine->AddOnScreenDebugMessage(2, 0.5f, FColor::Silver,
	                                 Game->GetCareerLine());
}

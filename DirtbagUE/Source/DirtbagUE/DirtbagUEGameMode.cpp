// Copyright Epic Games, Inc. All Rights Reserved.

#include "DirtbagUEGameMode.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "EngineUtils.h"

#include "DirtbagClimbWall.h"
#include "DirtbagGameInstance.h"
#include "DirtbagHUD.h"
#include "DirtbagRng.h"

DEFINE_LOG_CATEGORY_STATIC(LogDirtbagSim, Log, All);

ADirtbagUEGameMode::ADirtbagUEGameMode()
{
	HUDClass = ADirtbagHUD::StaticClass();
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

	// Where are we, before anyone has walked up to anything? Walls declare
	// their own venue, so a level whose walls all agree simply is that
	// place — and saying "indoors" while the player stands in front of a
	// crag is wrong from the first frame to whenever they first touch a
	// wall. Mixed levels keep the default and wait to be told by arrival,
	// which is the only honest answer there.
	if (UDirtbagGameInstance* Game =
	        Cast<UDirtbagGameInstance>(GetGameInstance()))
	{
		bool bFound = false;
		bool bUnanimous = true;
		EDirtbagVenue Agreed = EDirtbagVenue::Gym;
		for (TActorIterator<ADirtbagClimbWall> It(GetWorld()); It; ++It)
		{
			if (!bFound)
			{
				Agreed = It->GetVenue();
				bFound = true;
			}
			else if (It->GetVenue() != Agreed)
			{
				bUnanimous = false;
				break;
			}
		}
		if (bFound && bUnanimous)
		{
			Game->SetVenue(Agreed);
		}
	}

	// A missing HUD is otherwise a silent blank screen: say so plainly.
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && !Cast<ADirtbagHUD>(PC->GetHUD()))
	{
		UE_LOG(LogDirtbagSim, Warning,
		       TEXT("Dirtbag HUD is not active - set HUD Class to DirtbagHUD in "
		            "the game mode blueprint's Class Defaults."));
	}
}

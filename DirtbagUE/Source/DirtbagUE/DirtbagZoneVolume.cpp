#include "DirtbagZoneVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ADirtbagZoneVolume::ADirtbagZoneVolume()
{
	// It answers an overlap and nothing else. No keys, no frames.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetupAttachment(Root);
	// Big by default, because a zone is a district and not a doorway --
	// and because a volume that has to be resized before it does anything
	// is a volume somebody forgets to resize.
	Bounds->SetBoxExtent(FVector(4000.f, 4000.f, 1000.f));
}

void ADirtbagZoneVolume::BeginPlay()
{
	Super::BeginPlay();
	Game = Cast<UDirtbagGameInstance>(GetGameInstance());
	Bounds->OnComponentBeginOverlap.AddDynamic(this,
	                                           &ADirtbagZoneVolume::OnEnter);

	// **The one you are standing in at spawn counts.** Without this a
	// career starts in whatever zone the game instance defaulted to until
	// the player walks out and back in again -- and since a career starts
	// at the Lot and the default is the Lot, it would be right by accident
	// and wrong the first time anybody moves the player start.
	if (Game && GetWorld())
	{
		if (const APawn* Pawn =
		        UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (Bounds->IsOverlappingActor(Pawn))
			{
				Game->CurrentZone = Zone;
			}
		}
	}
}

void ADirtbagZoneVolume::OnEnter(UPrimitiveComponent*, AActor* Other,
                                 UPrimitiveComponent*, int32, bool,
                                 const FHitResult&)
{
	if (!Game || Other != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}
	// **Crossing a border is what this is.** Overlapping volumes are fine
	// and expected -- the last one entered wins, which is what walking from
	// one district into another means.
	Game->CurrentZone = Zone;

	if (!bAnnounceOnEntry || Game->Player.Day == AnnouncedOnDay)
	{
		return;
	}
	AnnouncedOnDay = Game->Player.Day;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
		    -1, 5.f, FColor::Silver,
		    FString::Printf(TEXT("%s.  %s"), *Game->ZoneNameOf(Zone),
		                    *Game->ZoneBlurbOf(Zone)));
	}
}

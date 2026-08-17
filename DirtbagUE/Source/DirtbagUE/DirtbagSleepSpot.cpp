#include "DirtbagSleepSpot.h"

#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "DirtbagGameInstance.h"

ADirtbagSleepSpot::ADirtbagSleepSpot()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetBoxExtent(FVector(200.f, 200.f, 120.f));
}

void ADirtbagSleepSpot::BeginPlay()
{
	Super::BeginPlay();
	Game = Cast<UDirtbagGameInstance>(GetGameInstance());
	Trigger->OnComponentBeginOverlap.AddDynamic(
	    this, &ADirtbagSleepSpot::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(
	    this, &ADirtbagSleepSpot::OnTriggerEnd);
}

void ADirtbagSleepSpot::OnTriggerBegin(UPrimitiveComponent*, AActor* OtherActor,
                                       UPrimitiveComponent*, int32, bool,
                                       const FHitResult&)
{
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0) || !Game)
	{
		return;
	}
	bPlayerNear = true;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
		    -1, 4.f, FColor::Cyan,
		    FString::Printf(TEXT("Call it a day?  (E to sleep)  —  day %d, $%.0f"),
		                    Game->Player.Day, Game->Player.Cash));
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		if (InputComponent && !bBoundInput)
		{
			InputComponent->BindKey(EKeys::E, IE_Pressed, this,
			                        &ADirtbagSleepSpot::OnInteract);
			bBoundInput = true;
		}
	}
}

void ADirtbagSleepSpot::OnTriggerEnd(UPrimitiveComponent*, AActor* OtherActor,
                                     UPrimitiveComponent*, int32)
{
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}
	bPlayerNear = false;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		DisableInput(PC);
	}
}

void ADirtbagSleepSpot::OnInteract()
{
	if (!bPlayerNear || !Game)
	{
		return;
	}
	Game->Sleep();
	if (GEngine)
	{
		// The morning report, dry as the chalk bag.
		GEngine->AddOnScreenDebugMessage(
		    -1, 6.f, FColor::Yellow,
		    FString::Printf(
		        TEXT("Day %d.  $%.0f.  Skin %.1f.  Saved."), Game->Player.Day,
		        Game->Player.Cash, Game->Player.Climber.Skin));
	}
}

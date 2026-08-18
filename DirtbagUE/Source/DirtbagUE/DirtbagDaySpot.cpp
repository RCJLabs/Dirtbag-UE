#include "DirtbagDaySpot.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "DirtbagGameInstance.h"

namespace
{
void Say(const FString& Msg, FColor Color, float Seconds = 4.f)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Seconds, Color, Msg);
	}
}
}  // namespace

ADirtbagDaySpot::ADirtbagDaySpot()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetBoxExtent(FVector(200.f, 200.f, 120.f));
}

void ADirtbagDaySpot::BeginPlay()
{
	Super::BeginPlay();
	Game = Cast<UDirtbagGameInstance>(GetGameInstance());
	Trigger->OnComponentBeginOverlap.AddDynamic(this,
	                                            &ADirtbagDaySpot::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(this,
	                                          &ADirtbagDaySpot::OnTriggerEnd);
}

FString ADirtbagDaySpot::PromptText() const
{
	if (!Game)
	{
		return FString();
	}
	switch (Kind)
	{
	case EDirtbagSpotKind::Meal:
		return FString::Printf(TEXT("Eat something?  (E)  -  hunger %.0f, $%.0f"),
		                       Game->Day.Hunger, Game->Player.Cash);
	case EDirtbagSpotKind::Shift:
		return FString::Printf(TEXT("Take a shift?  (E)  -  it's %.0f:00, $%.0f"),
		                       Game->Day.Hour, Game->Player.Cash);
	case EDirtbagSpotKind::Sleep:
		return FString::Printf(TEXT("Call it a day?  (E)  -  day %d, $%.0f"),
		                       Game->Player.Day, Game->Player.Cash);
	case EDirtbagSpotKind::Travel:
		return FString::Printf(TEXT("Drive to %s?  (E)  -  %.0f minutes"),
		                       *TravelName, TravelHours * 60.0);
	case EDirtbagSpotKind::Rest:
	{
		// The prompt carries the forecast, because that is the entire
		// reason anybody sits down here.
		const double Until = Game->HoursUntilWindow();
		if (bWaitForWindow && Until > 0.0)
		{
			return FString::Printf(
			    TEXT("Sit and wait?  (E)  -  %s"), *Game->WaitAdvice());
		}
		return FString::Printf(TEXT("Sit a while?  (E)  -  %.0f minutes.  %s"),
		                       RestHours * 60.0, *Game->WaitAdvice());
	}
	}
	return FString();
}

void ADirtbagDaySpot::OnTriggerBegin(UPrimitiveComponent*, AActor* OtherActor,
                                     UPrimitiveComponent*, int32, bool,
                                     const FHitResult&)
{
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0) || !Game)
	{
		return;
	}
	bPlayerNear = true;
	Say(PromptText(), FColor::Cyan);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		if (InputComponent && !bBoundInput)
		{
			InputComponent->BindKey(EKeys::E, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnInteract);
			bBoundInput = true;
		}
	}
}

void ADirtbagDaySpot::OnTriggerEnd(UPrimitiveComponent*, AActor* OtherActor,
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

void ADirtbagDaySpot::OnInteract()
{
	if (!bPlayerNear || !Game)
	{
		return;
	}

	switch (Kind)
	{
	case EDirtbagSpotKind::Meal:
	{
		const double Before = Game->Player.Cash;
		if (Game->EatMeal())
		{
			Say(FString::Printf(TEXT("Ate. -$%.0f, hunger %.0f."),
			                    Before - Game->Player.Cash, Game->Day.Hunger),
			    FColor::Green);
		}
		else
		{
			// The dirtbag's oldest problem, stated without comment.
			Say(TEXT("Not enough for that."), FColor::Orange);
		}
		break;
	}
	case EDirtbagSpotKind::Shift:
	{
		const double Before = Game->Player.Cash;
		Game->WorkShift();
		Say(FString::Printf(TEXT("Shift done. +$%.0f. It's %.0f:00, energy %.0f."),
		                    Game->Player.Cash - Before, Game->Day.Hour,
		                    Game->Day.Energy),
		    FColor::Green);
		break;
	}
	case EDirtbagSpotKind::Travel:
	{
		BeginDrive();
		break;
	}
	case EDirtbagSpotKind::Rest:
	{
		// Waiting is only interesting when it is waiting *for* something,
		// so a press skips straight to the moment the rock comes good
		// rather than making you press it eight more times.
		const double Until = Game->HoursUntilWindow();
		const double Hours =
		    (bWaitForWindow && Until > 0.0) ? Until : RestHours;
		Game->Rest(Hours);
		Say(FString::Printf(TEXT("%s  %s"),
		                    Hours >= 1.0
		                        ? *FString::Printf(TEXT("Sat for %.1f hours."),
		                                           Hours)
		                        : *FString::Printf(TEXT("Sat for %.0f minutes."),
		                                           Hours * 60.0),
		                    *Game->WaitAdvice()),
		    FColor::Cyan, 5.f);
		break;
	}
	case EDirtbagSpotKind::Sleep:
	{
		Game->Sleep();
		Say(FString::Printf(TEXT("Day %d.  $%.0f.  Skin %.1f.  Saved."),
		                    Game->Player.Day, Game->Player.Cash,
		                    Game->Player.Climber.Skin),
		    FColor::Yellow, 6.f);
		Say(Game->GetCareerLine(), FColor::Silver, 6.f);
		break;
	}
	}
}

void ADirtbagDaySpot::BeginDrive()
{
	if (!TravelTarget)
	{
		Say(TEXT("This drive has no Travel Target set."), FColor::Red);
		return;
	}

	// Fade out, let the clock run, arrive. The drive is time and a change of
	// place; the van earns its opinions about both in Phase 3.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeSeconds,
			                                         FLinearColor::Black, false,
			                                         true);
		}
	}
	GetWorldTimerManager().SetTimer(DriveTimer, this,
	                                &ADirtbagDaySpot::ArriveFromDrive,
	                                FMath::Max(0.05f, FadeSeconds), false);
}

void ADirtbagDaySpot::ArriveFromDrive()
{
	if (!Game || !TravelTarget)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (APawn* Pawn = PC ? PC->GetPawn() : nullptr)
	{
		const FVector Where = TravelTarget->GetActorLocation();
		Pawn->TeleportTo(Where, Pawn->GetActorRotation());
		if (PC)
		{
			// Face the way the destination faces, so arrivals are composed.
			PC->SetControlRotation(TravelTarget->GetActorRotation());
		}
	}

	Game->PassHours(TravelHours);
	Game->SetVenue(ArriveAt);

	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeSeconds,
		                                         FLinearColor::Black, false,
		                                         false);
	}
	Say(FString::Printf(TEXT("Drove to %s. %.0f minutes gone."), *TravelName,
	                    TravelHours * 60.0),
	    FColor::Silver);
}

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
		                       *TravelName, DriveHours() * 60.0);
	case EDirtbagSpotKind::Dog:
		return FString::Printf(TEXT("Feed it?  (E)  -  %s.  $%.0f"),
		                       *Game->DogLine(), Game->Player.Cash);
	case EDirtbagSpotKind::Van:
	{
		const FString What = Game->VanLine();
		return What.IsEmpty()
		           ? FString::Printf(TEXT("Look at the van?  (E)  -  nothing "
		                                  "wrong with it"))
		           : FString::Printf(TEXT("Sort the van?  (E)  -  %s.  $%.0f"),
		                             *What, Game->Player.Cash);
	}
	case EDirtbagSpotKind::GearShop:
	{
		// Say what it is costing you once it is costing you anything. Dead
		// rubber is the cheapest real handicap in the game and the only one
		// the player had no way to see.
		// Say what is on offer before it is bought, not after.
		const FString Pad = Game->PadOfferLine();
		if (Game->Player.Shoes.Wear < 0.3 && !Pad.IsEmpty())
		{
			return FString::Printf(TEXT("%s  (E)  -  $%.0f"), *Pad,
			                       Game->Player.Cash);
		}
		const double Grades = Game->ShoeCostInGrades();
		return Grades >= 0.05
		    ? FString::Printf(
		          TEXT("Shoes?  (E)  -  %s, costing you %.1f of a grade.  $%.0f"),
		          *Game->ShoeLine(), Grades, Game->Player.Cash)
		    : FString::Printf(TEXT("Shoes?  (E)  -  %s.  $%.0f"),
		                      *Game->ShoeLine(), Game->Player.Cash);
	}
	case EDirtbagSpotKind::Fire:
	{
		// The fire's prompt names who is here, because that is what makes
		// it different from sitting on a rock on your own.
		FString Who;
		for (const FDirtbagPartner& P : Game->GetLot())
		{
			if (!Who.IsEmpty())
			{
				Who += TEXT(", ");
			}
			Who += P.Name;
		}
		return FString::Printf(TEXT("Sit at the fire?  (E)  -  %s.  %s"),
		                       *Who, *Game->WaitAdvice());
	}
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
	case EDirtbagSpotKind::Van:
	{
		if (Game->WorstVanPart() < 0)
		{
			Say(TEXT("Nothing wrong with it worth the afternoon."),
			    FColor::Silver);
			break;
		}
		// Best repair you can actually afford, which is the decision when
		// you are broke and the honest default when you are not.
		if (Game->ReplaceVanPart())
		{
			Say(FString::Printf(TEXT("Done properly.  $%.0f left."),
			                    Game->Player.Cash),
			    FColor::Green, 6.f);
		}
		else if (Game->PatchVan())
		{
			Say(FString::Printf(TEXT("Patched. It will hold a while.  $%.0f "
			                         "left."), Game->Player.Cash),
			    FColor::Yellow, 6.f);
		}
		else
		{
			Game->BodgeVan();
			Say(TEXT("Four hours under it with a spanner. It will do."),
			    FColor::Orange, 6.f);
		}
		break;
	}
	case EDirtbagSpotKind::GearShop:
	{
		if (Game->Player.Shoes.Wear < 0.3)
		{
			// Rubber is fine, so the shop's other business. The pad is the
			// purchase measured to move a season most and it was reachable
			// only from a Blueprint call before this.
			const FString Pad = Game->PadOfferLine();
			if (!Pad.IsEmpty())
			{
				if (Game->BuyCrashPad())
				{
					Say(FString::Printf(
					        TEXT("Second pad in the van.  $%.0f left."),
					        Game->Player.Cash),
					    FColor::Green, 6.f);
				}
				else
				{
					Say(FString::Printf(TEXT("%s  Not today."), *Pad),
					    FColor::Orange, 6.f);
				}
				break;
			}
			Say(TEXT("Your shoes are fine. Keep your money."), FColor::Silver);
			break;
		}
		if (Game->ResoleShoes())
		{
			Say(FString::Printf(TEXT("Resoled.  $%.0f left."),
			                    Game->Player.Cash),
			    FColor::Green, 6.f);
		}
		else if (Game->BuyNewShoes())
		{
			Say(FString::Printf(TEXT("New rubber.  $%.0f left."),
			                    Game->Player.Cash),
			    FColor::Green, 6.f);
		}
		else
		{
			Say(TEXT("Not this week."), FColor::Orange);
		}
		break;
	}
	case EDirtbagSpotKind::Dog:
	{
		const bool bWasStray = !Game->Player.Dog.bAdopted;
		if (!Game->FeedTheDog())
		{
			Say(TEXT("Not enough for that, and it knows."), FColor::Orange);
			break;
		}
		if (bWasStray && Game->Player.Dog.bAdopted)
		{
			// No ceremony anywhere else, so none here either. It just
			// stops being a stray and starts being yours.
			Say(TEXT("It follows you back to the van and lies down."),
			    FColor::Yellow, 7.f);
		}
		else
		{
			Say(FString::Printf(TEXT("Fed it.  %s"), *Game->DogLine()),
			    FColor::Green);
		}
		break;
	}
	case EDirtbagSpotKind::Fire:
	{
		const double Until = Game->HoursUntilWindow();
		const double Hours =
		    (bWaitForWindow && Until > 0.0) ? Until : RestHours;
		const FString Heard = Game->SitAtTheFire(Hours);
		Say(Heard.IsEmpty()
		        ? FString::Printf(TEXT("Sat at the fire for %.1f hours."),
		                          Hours)
		        : FString::Printf(TEXT("%.1f hours at the fire.  %s"), Hours,
		                          *Heard),
		    FColor::Yellow, 6.f);
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

double ADirtbagDaySpot::DriveHours() const
{
	if (Game)
	{
		UDirtbagGameInstance* G = const_cast<UDirtbagGameInstance*>(Game.Get());

		// A drive costs the approach of whichever end of it is rock, so the
		// way back is the same length as the way out. Asking only about the
		// destination would make the cave forty minutes to reach and half
		// an hour to leave, because the Lot is at Roadside.
		const double There = G->ApproachHoursFor(ArriveAt);
		if (There > 0.0)
		{
			return There;
		}
		const double Here = G->ApproachHoursFor(G->Venue);
		if (Here > 0.0)
		{
			return Here;
		}
	}
	return TravelHours;
}

void ADirtbagDaySpot::BeginDrive()
{
	if (!TravelTarget)
	{
		Say(TEXT("This drive has no Travel Target set."), FColor::Red);
		return;
	}

	// A broken van does not go anywhere, and this is the one place in the
	// game that says no. It is not a lock: bodging is free and always
	// works, so the way out is four hours rather than money.
	if (Game && !Game->VanRuns())
	{
		Say(FString::Printf(TEXT("%s  (E at the van to sort it)"),
		                    *Game->VanLine()),
		    FColor::Red, 6.f);
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

	// The drive itself: hours on the clock, hours on the van, and the
	// chance that the thing you have been ignoring picks this morning.
	const double Hours = DriveHours();
	const int32 Broke = Game->DriveVan(Hours);
	Game->PassHours(Hours);
	Game->SetVenue(ArriveAt);

	if (Broke >= 0)
	{
		Say(FString::Printf(TEXT("%s  %s"), *Game->VanNews, *Game->VanLine()),
		    FColor::Red, 8.f);
	}

	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeSeconds,
		                                         FLinearColor::Black, false,
		                                         false);
	}
	Say(FString::Printf(TEXT("Drove to %s. %.0f minutes and $%.0f gone."),
	                    *TravelName, Hours * 60.0, Game->LastDriveFuel),
	    FColor::Silver);
}

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
		// The dream counter, before the rubber talk: naming what the money
		// is for happens once a career, and until it is named the shop has
		// nothing bigger to say.
		if (Game->Player.Dreams.Chosen == EDirtbagDream::None)
		{
			return FString::Printf(
			    TEXT("What is the money for?  1 the Rig $%.0f   2 the War "
			         "Chest $%.0f   3 Home Base $%.0f   -  once, and it "
			         "holds.  $%.0f"),
			    Game->DreamCost(EDirtbagDream::Rig),
			    Game->DreamCost(EDirtbagDream::WarChest),
			    Game->DreamCost(EDirtbagDream::HomeBase), Game->Player.Cash);
		}
		// Chosen and not yet bought: the shop keeps the number in your
		// eyeline, and E hands the money over the day you have it -- but
		// worn rubber still talks first, because the dream can wait a
		// resole and the resole cannot wait a dream.
		const bool bOwnsIt =
		    (Game->Player.Dreams.Chosen == EDirtbagDream::Rig &&
		     Game->Player.Dreams.bRig) ||
		    (Game->Player.Dreams.Chosen == EDirtbagDream::WarChest &&
		     Game->Player.Dreams.bWarChest) ||
		    (Game->Player.Dreams.Chosen == EDirtbagDream::HomeBase &&
		     Game->Player.Dreams.bHomeBase);
		if (Game->Player.Shoes.Wear < 0.3 && !bOwnsIt)
		{
			const double Cost = Game->DreamCost(Game->Player.Dreams.Chosen);
			return Game->CanAffordDream(Game->Player.Dreams.Chosen)
			           ? FString::Printf(
			                 TEXT("Buy %s?  (E)  -  $%.0f, leaving $%.0f"),
			                 *Game->DreamName(Game->Player.Dreams.Chosen),
			                 Cost, Game->Player.Cash - Cost)
			           : FString::Printf(
			                 TEXT("%s  (E for shoes)  -  $%.0f of $%.0f"),
			                 *Game->DreamName(Game->Player.Dreams.Chosen),
			                 Game->Player.Cash, Cost);
		}
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
		// Which game is out tonight rotates with the day: you join what is
		// being played, you do not order off a menu.
		const TCHAR* Tonight[3] = {TEXT("cards"), TEXT("dice"),
		                           TEXT("blackjack")};
		return FString::Printf(
		    TEXT("Sit at the fire?  (E)  -  %s.  %s out tonight (C).  %s"),
		    *Who, Tonight[Game->Player.Day % 3], *Game->WaitAdvice());
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
			InputComponent->BindKey(EKeys::C, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnCommit);
			InputComponent->BindKey(EKeys::F, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnBackDown);
			InputComponent->BindKey(EKeys::One, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnChoose1);
			InputComponent->BindKey(EKeys::Two, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnChoose2);
			InputComponent->BindKey(EKeys::Three, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnChoose3);
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
		// The dream, when today is the day. Shoes still outrank it when
		// they are worn -- but a chosen, affordable dream outranks the pad
		// pitch, because the shop does not upsell somebody who came in to
		// buy a van.
		if (Game->Player.Shoes.Wear < 0.3 &&
		    Game->Player.Dreams.Chosen != EDirtbagDream::None &&
		    Game->CanAffordDream(Game->Player.Dreams.Chosen))
		{
			if (Game->BuyDream(Game->Player.Dreams.Chosen))
			{
				Say(Game->DreamNews, FColor::Yellow, 8.f);
				break;
			}
		}
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

void ADirtbagDaySpot::OnCommit()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Fire)
	{
		return;
	}

	// Hands are keyed on (day, number), so a new day is a new evening --
	// and a reload mid-evening re-deals the identical hand, which is the
	// same no-reroll rule every gamble in this game lives under.
	if (HandDay != Game->Player.Day)
	{
		HandDay = Game->Player.Day;
		HandNumber = 0;
		bHandPending = false;
	}

	const int32 Tonight = Game->Player.Day % 3;
	if (!bHandPending)
	{
		HandNumber++;
		bHandPending = true;
		switch (Tonight)
		{
		case 0:   // cards
		{
			const FDirtbagCampfireHand Hand =
			    Game->DealCampfireHand(HandNumber);
			FString Reads;
			for (const FDirtbagCampfireRead& R : Hand.Reads)
			{
				Reads += TEXT("\n") + R.Tell;
			}
			Say(FString::Printf(TEXT("Your hand: %.0f of 100.  Pot $%.0f.%s"
			                         "\nStay (C, $%.0f more) or throw them "
			                         "in (F)."),
			                    Hand.Yours * 100.0, Hand.Pot, *Reads,
			                    CardStake),
			    FColor::White, 12.f);
			break;
		}
		case 1:   // liar's dice
		{
			const FDirtbagLiarsDice R = Game->DealLiarsDice(HandNumber);
			FString Cup;
			for (int32 Pip : R.Yours)
			{
				Cup += FString::Printf(TEXT(" %d"), Pip);
			}
			Say(FString::Printf(TEXT("Under your cup:%s  (%d dice on the "
			                         "table, ones wild)\n%s\n%s\nCall it "
			                         "(C, $%.0f) or let it go round (F)."),
			                    *Cup, R.DiceOnTable, *R.Bid, *R.Tell,
			                    CardStake),
			    FColor::White, 12.f);
			break;
		}
		default:   // blackjack
		{
			const FDirtbagBlackjack H = Game->DealBlackjack(HandNumber);
			Say(FString::Printf(TEXT("You: %d.  The deck shows %d.\nAnother "
			                         "card (C) or stick (F, $%.0f down)."),
			                    H.Yours, H.DealerShows, CardStake),
			    FColor::White, 10.f);
			break;
		}
		}
		return;
	}

	// A hand is live: C commits.
	switch (Tonight)
	{
	case 0:
	{
		const FString Line =
		    Game->PlayCampfireHand(HandNumber, CardStake, false);
		Say(FString::Printf(TEXT("%s  %+.0f.  $%.0f in the pocket.  "
		                         "Again (C)?"),
		                    *Line, Game->LastHandCash, Game->Player.Cash),
		    Game->LastHandCash >= 0.0 ? FColor::Green : FColor::Orange, 8.f);
		bHandPending = false;
		break;
	}
	case 1:
	{
		const FString Line = Game->PlayLiarsDice(HandNumber, CardStake, true);
		Say(FString::Printf(TEXT("%s  %+.0f.  $%.0f in the pocket.  "
		                         "Again (C)?"),
		                    *Line, Game->LastHandCash, Game->Player.Cash),
		    Game->LastHandCash >= 0.0 ? FColor::Green : FColor::Orange, 8.f);
		bHandPending = false;
		break;
	}
	default:
	{
		// Blackjack's C is another card, not a settlement -- the hand
		// stays live until you stick (F) or go over.
		const int32 Card = Game->HitBlackjack(HandNumber);
		if (Game->LastBlackjack.bBust)
		{
			// Going over settles at once; there is nothing left to decide.
			const FString Line =
			    Game->StandBlackjack(HandNumber, CardStake);
			Say(FString::Printf(TEXT("Drew %d.  %s  %+.0f.  $%.0f in the "
			                         "pocket.  Again (C)?"),
			                    Card, *Line, Game->LastHandCash,
			                    Game->Player.Cash),
			    FColor::Orange, 8.f);
			bHandPending = false;
		}
		else
		{
			Say(FString::Printf(TEXT("Drew %d.  You: %d.  Another (C) or "
			                         "stick (F)?"),
			                    Card, Game->LastBlackjack.Yours),
			    FColor::White, 10.f);
		}
		break;
	}
	}
}

void ADirtbagDaySpot::OnBackDown()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Fire ||
	    !bHandPending || HandDay != Game->Player.Day)
	{
		return;
	}
	const int32 Tonight = Game->Player.Day % 3;
	FString Line;
	switch (Tonight)
	{
	case 0:
		Line = Game->PlayCampfireHand(HandNumber, 0.0, true);
		break;
	case 1:
		Line = Game->PlayLiarsDice(HandNumber, 0.0, false);
		break;
	default:
		Line = Game->StandBlackjack(HandNumber, CardStake);
		break;
	}
	Say(FString::Printf(TEXT("%s  %+.0f.  $%.0f in the pocket.  Again (C)?"),
	                    *Line, Game->LastHandCash, Game->Player.Cash),
	    Game->LastHandCash >= 0.0 ? FColor::Green : FColor::Orange, 8.f);
	bHandPending = false;
}

void ADirtbagDaySpot::OnChoose1() { ChooseDreamAt(EDirtbagDream::Rig); }
void ADirtbagDaySpot::OnChoose2() { ChooseDreamAt(EDirtbagDream::WarChest); }
void ADirtbagDaySpot::OnChoose3() { ChooseDreamAt(EDirtbagDream::HomeBase); }

void ADirtbagDaySpot::ChooseDreamAt(EDirtbagDream Which)
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop)
	{
		return;
	}
	// ChooseDream itself refuses a second choice; the shop only adds the
	// words. Said back with the blurb, because this is the one purchase
	// conversation in the game that is actually about the next ten years.
	if (Game->ChooseDream(Which))
	{
		Say(FString::Printf(TEXT("%s, then.  %s"), *Game->DreamName(Which),
		                    *Game->DreamBlurb(Which)),
		    FColor::Yellow, 8.f);
	}
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

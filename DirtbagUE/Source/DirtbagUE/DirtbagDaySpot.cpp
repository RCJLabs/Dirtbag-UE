#include "DirtbagDaySpot.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
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
	{
		// The verb is the whole difference between the two travel rules, so
		// the prompt says which one this is rather than calling a walk a
		// drive. A walk also says what it does not cost, because "no fuel"
		// is the reason you would choose it on a week when the van is sick.
		if (!UDirtbagSimLibrary::NeedsTheVan(DestinationZone))
		{
			return FString::Printf(
			    TEXT("Walk to %s?  (E)  -  %.0f minutes, no van needed"),
			    *TravelName, WalkHours() * 60.0);
		}
		return FString::Printf(TEXT("Drive to %s?  (E)  -  %.0f minutes"),
		                       *TravelName, DriveHours() * 60.0);
	}
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
		// being played, you do not order off a menu. The rule lives in the
		// sim -- this used to be the fourth hand-written copy of `% 3`,
		// with its own separate list of names to get out of step.
		return FString::Printf(
		    TEXT("Sit at the fire?  (E)  -  %s.  %s out tonight (C).  %s"),
		    *Who, *Game->FiresideGameName(Game->WhatsOutTonight()),
		    *Game->WaitAdvice());
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
	// The prompt used to be a four-second toast said once. It is the
	// interaction, not news about it: it stays up now for as long as you
	// are standing here, and is rebuilt whenever anything it describes
	// changes.
	PushPrompt();
	if (Kind == EDirtbagSpotKind::Fire)
	{
		StakeNotch = FMath::Clamp(StartingStakeNotch, 0, 2);
		RefreshFireTable();
	}

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
	PushPrompt();
	// Standing up mid-hand is not a free look. Every game here takes the
	// ante at settlement rather than at the deal, so walking away used to
	// cost nothing at all: deal, read the table, leave.
	SettleAndLeave();
	RefreshFireTable();
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
		// Not while you are holding cards. Sitting passes hours, hours can
		// cross midnight, and a hand keyed on yesterday's day would simply
		// stop existing -- unsettled, unpaid, and gone from the table with
		// nothing said. Finish the hand first; that is what you would do.
		if (bHandPending && HandDay == Game->Player.Day)
		{
			Say(TEXT("Finish the hand first."), FColor::Silver, 4.f);
			return;
		}
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
		// The hours may have crossed midnight into a different game and a
		// clean evening, so the table is rebuilt rather than left saying
		// what was out last night.
		RefreshFireTable();
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

	// The one verb, heard. Fired here at the single exit rather than in
	// each branch, so it follows the same rule the prompt does: whatever
	// the press did, it made a noise.
	if (InteractSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InteractSound,
		                                      GetActorLocation());
	}

	// Whatever just happened changed what this spot can do for you next --
	// the shop that sold you shoes now has a pad to pitch, the van that was
	// broken now runs, the dog is fed. One call, at the one exit, because a
	// prompt refreshed in eight branches is a prompt stale in the ninth.
	PushPrompt();
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

void ADirtbagDaySpot::PushPrompt()
{
	if (!Game)
	{
		return;
	}
	if (!bPlayerNear)
	{
		Game->ClearPrompt(this);
		return;
	}
	FDirtbagPromptLine Line;
	Line.Text = PromptText();
	// Only one spot state is a hard block rather than a price: a van that
	// does not run stops both driving it and being anywhere else today.
	// The rest of the prompts say their own state in words -- "$140 of
	// $9,000", "Not enough for that" -- and a colour on top of a sentence
	// that already says it is decoration.
	const bool bStranded =
	    !Game->VanRuns() &&
	    (Kind == EDirtbagSpotKind::Van ||
	     (Kind == EDirtbagSpotKind::Travel &&
	      UDirtbagSimLibrary::NeedsTheVan(DestinationZone)));
	Line.Tone = bStranded ? EDirtbagPromptTone::Blocked
	                      : EDirtbagPromptTone::Plain;
	TArray<FDirtbagPromptLine> Lines;
	if (!Line.Text.IsEmpty())
	{
		Lines.Add(Line);
	}
	Game->SetPrompt(this, Lines);
}

void ADirtbagDaySpot::RefreshFireTable()
{
	if (!Game)
	{
		return;
	}
	FDirtbagFireReadout& T = Game->FireReadout;
	T.bActive = bPlayerNear && Kind == EDirtbagSpotKind::Fire;
	if (!T.bActive)
	{
		T.bHandLive = false;
		return;
	}

	const bool bSameEvening = HandDay == Game->Player.Day;
	T.Tonight = Game->WhatsOutTonight();
	T.GameLine = Game->FiresideGameName(T.Tonight).ToUpper();
	T.StakeNotch = StakeNotch;
	T.bHandLive = bHandPending && bSameEvening;
	// The stake on show is the one that would actually be played: the live
	// one while a hand is up, the selected one while you are deciding
	// whether to deal. Showing the selected notch during a locked blackjack
	// hand would be a lie about what is on the table.
	T.Stake = T.bHandLive ? LiveStake : Game->FireStake(StakeNotch);
	T.HandsTonight = bSameEvening ? HandsSettled : 0;
	T.NightDelta = bSameEvening ? NightDelta : 0.0;
	if (!bSameEvening)
	{
		// A new evening starts clean, including the last hand's sentence --
		// what happened last night is not news at this fire.
		T.LastLine.Reset();
		T.LastDelta = 0.0;
	}

	T.Reads.Reset();
	T.Yours = -1.0;
	T.YoursLine.Reset();
	T.TableLine.Reset();

	switch (T.Tonight)
	{
	case EDirtbagFiresideGame::Cards:
		T.CommitVerb = TEXT("stay in");
		T.BackVerb = TEXT("throw them in");
		if (T.bHandLive)
		{
			const FDirtbagCampfireHand H = Game->DealCampfireHand(HandNumber);
			T.Yours = H.Yours;
			T.TableLine = FString::Printf(TEXT("Pot $%.0f"), H.Pot);
			for (const FDirtbagCampfireRead& R : H.Reads)
			{
				T.Reads.Add(R.Tell);
			}
		}
		break;

	case EDirtbagFiresideGame::Dice:
		T.CommitVerb = TEXT("call it");
		T.BackVerb = TEXT("let it go round");
		if (T.bHandLive)
		{
			const FDirtbagLiarsDice R = Game->DealLiarsDice(HandNumber);
			FString Cup;
			for (int32 Pip : R.Yours)
			{
				Cup += FString::Printf(TEXT("  %d"), Pip);
			}
			T.YoursLine = FString::Printf(TEXT("Under your cup:%s"), *Cup);
			T.TableLine = FString::Printf(TEXT("%s   (%d dice out, ones wild)"),
			                              *R.Bid, R.DiceOnTable);
			T.Reads.Add(R.Tell);
		}
		break;

	default:
		T.CommitVerb = TEXT("another card");
		T.BackVerb = TEXT("stick");
		if (T.bHandLive)
		{
			const FDirtbagBlackjack& BJ = Game->LastBlackjack;
			// Against 21 rather than against nothing, so the bar says how
			// close to the edge you are -- which is the entire question
			// blackjack asks and the one a bare number makes you do
			// arithmetic for.
			T.Yours = static_cast<double>(BJ.Yours) / 21.0;
			T.YoursLine =
			    BJ.Draws > 0
			        ? FString::Printf(TEXT("You: %d, on %d more"), BJ.Yours,
			                          BJ.Draws)
			        : FString::Printf(TEXT("You: %d"), BJ.Yours);
			T.TableLine =
			    FString::Printf(TEXT("The deck shows %d"), BJ.DealerShows);
		}
		break;
	}
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
		HandsSettled = 0;
		NightDelta = 0.0;
	}

	const EDirtbagFiresideGame Tonight = Game->WhatsOutTonight();
	if (!bHandPending)
	{
		HandNumber++;
		bHandPending = true;
		LiveStake = Game->FireStake(StakeNotch);
		if (Tonight == EDirtbagFiresideGame::Blackjack)
		{
			// Deal it, so the table has a hand to show. Poker and dice
			// re-derive theirs; blackjack's is mutable state and lives on
			// the game instance.
			Game->DealBlackjack(HandNumber);
		}
		RefreshFireTable();
		return;
	}

	// A hand is live: C commits.
	switch (Tonight)
	{
	case EDirtbagFiresideGame::Cards:
		SettleFireHand(Game->PlayCampfireHand(HandNumber, LiveStake, false));
		break;

	case EDirtbagFiresideGame::Dice:
		SettleFireHand(Game->PlayLiarsDice(HandNumber, LiveStake, true));
		break;

	default:
	{
		// Blackjack's C is another card, not a settlement -- the hand
		// stays live until you stick (F) or go over.
		const int32 Card = Game->HitBlackjack(HandNumber);
		if (Game->LastBlackjack.bBust)
		{
			// Going over settles at once; there is nothing left to decide.
			SettleFireHand(
			    FString::Printf(TEXT("Drew %d.  %s"), Card,
			                    *Game->StandBlackjack(HandNumber, LiveStake)));
		}
		else
		{
			RefreshFireTable();
		}
		break;
	}
	}
}

void ADirtbagDaySpot::SettleFireHand(const FString& Line)
{
	if (!Game)
	{
		return;
	}
	FDirtbagFireReadout& T = Game->FireReadout;
	T.LastLine = Line;
	T.LastDelta = Game->LastHandCash;
	NightDelta += Game->LastHandCash;
	HandsSettled++;
	bHandPending = false;
	RefreshFireTable();
}

void ADirtbagDaySpot::OnBackDown()
{
	if (!bPlayerNear)
	{
		return;
	}
	SettleAndLeave();
}

void ADirtbagDaySpot::SettleAndLeave()
{
	if (!Game || Kind != EDirtbagSpotKind::Fire || !bHandPending ||
	    HandDay != Game->Player.Day)
	{
		return;
	}
	// One body for both F and standing up, because they do the same thing
	// to the hand. The difference is only who decided: pressing F is a
	// decision, walking away is what a hand does when the person holding it
	// stands up -- and every game here takes the ante at settlement rather
	// than at the deal, so leaving used to be a free look at the table.
	switch (Game->WhatsOutTonight())
	{
	case EDirtbagFiresideGame::Cards:
		SettleFireHand(Game->PlayCampfireHand(HandNumber, 0.0, true));
		break;
	case EDirtbagFiresideGame::Dice:
		SettleFireHand(Game->PlayLiarsDice(HandNumber, 0.0, false));
		break;
	default:
		// Blackjack's F is not backing down, it is stopping -- a real
		// settlement at the stake the hand was dealt for.
		SettleFireHand(Game->StandBlackjack(HandNumber, LiveStake));
		break;
	}
	// Walking away takes the table with it, so the sentence it just wrote
	// would be drawn for no frames at all. That one case still needs a
	// toast, which is exactly what a toast is for: news, after the fact,
	// with nothing left to decide.
	if (!bPlayerNear)
	{
		Say(FString::Printf(TEXT("%s  %+.0f."), *Game->FireReadout.LastLine,
		                    Game->FireReadout.LastDelta),
		    Game->FireReadout.LastDelta >= 0.0 ? FColor::Green : FColor::Orange,
		    6.f);
	}
}

bool ADirtbagDaySpot::SetStakeNotch(int32 Notch)
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Fire)
	{
		return false;
	}
	StakeNotch = FMath::Clamp(Notch, 0, 2);
	// Poker and liar's dice size the bet with the hand in front of you --
	// that is the decision the games are made of. Blackjack does not: the
	// bet is placed before the cards, and raising once you can see them is
	// not a game, so a live blackjack hand keeps the stake it was dealt
	// for and the change applies to the next one.
	if (bHandPending && HandDay == Game->Player.Day &&
	    Game->WhatsOutTonight() != EDirtbagFiresideGame::Blackjack)
	{
		LiveStake = Game->FireStake(StakeNotch);
	}
	RefreshFireTable();
	return true;
}

// The same three keys, and the same question in both places: how much of
// the float is this worth. At the shop it buys a life; at the fire it buys
// a hand.
void ADirtbagDaySpot::OnChoose1()
{
	if (!SetStakeNotch(0)) { ChooseDreamAt(EDirtbagDream::Rig); }
}
void ADirtbagDaySpot::OnChoose2()
{
	if (!SetStakeNotch(1)) { ChooseDreamAt(EDirtbagDream::WarChest); }
}
void ADirtbagDaySpot::OnChoose3()
{
	if (!SetStakeNotch(2)) { ChooseDreamAt(EDirtbagDream::HomeBase); }
}

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
		// The counter stops asking what the money is for and starts
		// counting it, so the standing prompt has to change with it.
		PushPrompt();
	}
}

void ADirtbagDaySpot::BeginDrive()
{
	if (!TravelTarget)
	{
		// A level placed wrong, not a thing that happened in the game. It
		// goes to the log where it outlives the playtest, and the toast
		// names the actor and lasts long enough to be read.
		UE_LOG(LogDirtbagSetup, Warning,
		       TEXT("%s: Travel spot has no TravelTarget set; the drive "
		            "cannot go anywhere."),
		       *GetName());
		Say(FString::Printf(TEXT("SETUP: %s has no Travel Target."),
		                    *GetName()),
		    FColor::Red, 30.f);
		return;
	}

	// A broken van does not go anywhere -- but only to the places that
	// needed it.
	//
	// This used to gate *every* travel spot, which is one rule where the 2D
	// game has two, and the difference is most of a week of play: a dead
	// van is supposed to cost you the crags, not the gym, the shop, the
	// diner and the shift as well. Taking the whole game away is not
	// pressure, it is a pause.
	if (Game && !Game->VanRuns() &&
	    UDirtbagSimLibrary::NeedsTheVan(DestinationZone))
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

double ADirtbagDaySpot::WalkHours() const
{
	// Which walk this is depends on where you are standing as well as
	// where you are going, and only the Lot and town are connected today,
	// so the answer is the one walk there is. Asked of the zone model
	// rather than typed here: when the town grows, the table grows in one
	// file and every spot pointing along it follows.
	const EDirtbagZone From = DestinationZone == EDirtbagZone::Town
	                              ? EDirtbagZone::Lot
	                              : EDirtbagZone::Town;
	const double Minutes =
	    UDirtbagSimLibrary::WalkMinutes(From, DestinationZone);
	// A destination the zone model says is unwalkable should never have
	// reached here, but if it does, fall back to the spot's own number
	// rather than teleporting the player for free.
	return Minutes > 0.0 ? Minutes / 60.0 : DriveHours();
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

	// A walk costs time and nothing else: no fuel, no wear, no breakdown
	// roll. That is the point of connected ground -- the van is what buys
	// you rock, and everything else is your legs.
	const bool bOnFoot = !UDirtbagSimLibrary::NeedsTheVan(DestinationZone);
	const double Hours = bOnFoot ? WalkHours() : DriveHours();
	const int32 Broke = bOnFoot ? -1 : Game->DriveVan(Hours);
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
	Say(bOnFoot
	        ? FString::Printf(TEXT("Walked to %s. %.0f minutes."), *TravelName,
	                          Hours * 60.0)
	        : FString::Printf(TEXT("Drove to %s. %.0f minutes and $%.0f gone."),
	                          *TravelName, Hours * 60.0, Game->LastDriveFuel),
	    FColor::Silver);
}

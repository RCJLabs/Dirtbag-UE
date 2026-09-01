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
	// A spot is a trigger volume that answers keys and does not need
	// frames -- except while the road is on screen, which is the one thing
	// here that animates. Allowed but switched off, and switched on for the
	// length of a trip only.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

// Which seat you are hiring for. **The desk before the setter** -- the
// cheaper hire before the dearer one, which is the order anybody would do
// it in, and it means one key set covers both jobs. Defined up here
// because the prompt and the keys have to agree about it.
static bool WhichSeatIsOpen(const FDirtbagGym& Gym, bool& bFrontDesk)
{
	if (!Gym.bFrontDesk) { bFrontDesk = true; return true; }
	if (!Gym.bSetter) { bFrontDesk = false; return true; }
	return false;
}

// And who is asking for more money, in the same order.
static bool WhoIsAsking(const UDirtbagGameInstance& Game, bool& bFrontDesk)
{
	if (Game.GymStaffIsAsking(true)) { bFrontDesk = true; return true; }
	if (Game.GymStaffIsAsking(false)) { bFrontDesk = false; return true; }
	return false;
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
	{
		// **Whose counter it is.** A diner with a name behind it is the
		// difference between a vending machine and a place you go -- and
		// the greeting is shown here rather than only after the fact,
		// because reading it does not spend it: the sim spends the memory
		// when you are actually in front of them.
		const FString Who = Game->WhoIsAtTheCounter(EDirtbagService::Meal);
		const FString Said = Game->WhatTheyWouldSay(EDirtbagService::Meal);
		FString Line = FString::Printf(
		    TEXT("Eat something?  (E)  -  hunger %.0f, $%.0f"),
		    Game->Day.Hunger, Game->Player.Cash);
		if (!Who.IsEmpty())
		{
			Line += FString::Printf(TEXT("\n   %s"), *Who);
			if (!Said.IsEmpty())
			{
				Line += FString::Printf(TEXT(": \"%s\""), *Said);
			}
		}
		return Line;
	}
	case EDirtbagSpotKind::Shift:
	{
		// The board, not a shift.
		//
		// "Take a shift? (E)" was a placeholder for this and it flattened
		// the whole system: every gig became the same gig, and the choice
		// the board exists to offer -- fast money against cheap hours,
		// and the one that pays best because nobody else will do it --
		// never reached the player at all.
		FString Line = FString::Printf(
		    TEXT("The board.  It's %.0f:00, $%.0f, energy %.0f."),
		    Game->Day.Hour, Game->Player.Cash, Game->Day.Energy);
		const TArray<FDirtbagOddJob> Board = Game->TodaysJobBoard();
		for (int32 i = 0; i < Board.Num() && i < 3; i++)
		{
			const FDirtbagOddJob& Gig = Board[i];
			// Pay, hours and energy, because those are the three things
			// you are actually choosing between. The van requirement is
			// said only when it bites -- a gig that needs a van you have
			// is not worth a word.
			Line += FString::Printf(
			    TEXT("\n   %d  %s  -  $%.0f, %.0fh, %.0f energy%s"), i + 1,
			    *Gig.Name, Gig.Pay, Gig.Hours, Gig.Energy,
			    (Gig.bNeedsVan && !Game->VanRuns()) ? TEXT("  (needs the van)")
			                                        : TEXT(""));
			// **What is going to come up on that shift**, said before you
			// take it, because the answer is part of taking it. What the
			// harder way *needs* is deliberately not a number: you know
			// whether you can do your job, and a threshold on the wall
			// would turn a decision into a skill check you can read off.
			const FString What = Game->ShiftMomentLine(Gig);
			if (!What.IsEmpty())
			{
				Line += FString::Printf(TEXT("\n        %s"), *What);
				Line += FString::Printf(TEXT("\n        %s  (hold W)"),
				                        *Game->TheHardWayLine(Gig));
			}
		}

		// What the trades know about you, and what that makes you.
		const FString Trade = Game->TradeLine();
		if (!Trade.IsEmpty())
		{
			Line += FString::Printf(TEXT("\n   %s"), *Trade);
		}
		const FString Crafts = Game->CraftLine();
		if (!Crafts.IsEmpty())
		{
			Line += FString::Printf(TEXT("\n   %s"), *Crafts);
		}
		// The permanent position, under the gigs and separated from them,
		// because it is a different kind of thing: the gigs are a day and
		// this is your week, every week, until you hand it back.
		Line += FString::Printf(TEXT("\n   %s"), *Game->SalaryLine());
		return Line;
	}
	case EDirtbagSpotKind::Sleep:
	{
		FString Line = FString::Printf(
		    TEXT("Call it a day?  (E)  -  day %d, $%.0f"), Game->Player.Day,
		    Game->Player.Cash);
		// **The offer, said in full and offered rather than urged.** It
		// states what it is and both keys, and nothing anywhere says which
		// one is right -- declining costs nothing and the prompt does not
		// hint otherwise. Walking away leaves it standing, because "I have
		// not decided" is a real answer to this one.
		if (!Game->RivalOffer.IsEmpty())
		{
			Line += FString::Printf(TEXT("\n   %s  (C) yes   (F) no"),
			                        *Game->RivalOffer);
		}
		return Line;
	}
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
		if (bRetireArmed)
		{
			return FString(TEXT("Stop climbing, for good?  R again to mean "
			                    "it.  Anything else walks away."));
		}
		// The hangboard lives over the side door, so this is where it gets
		// used. Only offered when there is something to offer: owned, and
		// not already hung on today.
		FString Board;
		if (Game->Player.Kit.bHangboard && !Game->Day.bHangboardDone)
		{
			Board = TEXT("\n   The board is over the door.  (H)");
		}

		const FString What = Game->VanLine();
		FString Line =
		    What.IsEmpty()
		        ? FString(TEXT("Look at the van?  (E)  -  nothing wrong "
		                       "with it"))
		        : FString::Printf(TEXT("Sort the van?  (E)  -  %s.  $%.0f"),
		                          *What, Game->Player.Cash);
		// The one place the game says anything about stopping, and it says
		// it only when the body has an opinion. `TimeToThinkAboutIt` is
		// never a command and never age alone -- it is a run of injuries,
		// or two grades off your best and past the age. Silent otherwise,
		// because a line offering retirement every night of a career is a
		// line you learn to stop reading.
		if (Game->TimeToThinkAboutIt())
		{
			Line += TEXT("\n   You have been thinking about stopping.  (R)");
		}
		// And an unnamed climber can put a name to themselves here, which
		// is the only place in the game that was ever possible.
		if (Game->ClimberName.IsEmpty())
		{
			Line += TEXT("\n   Nobody has asked your name.  (R)");
		}
		return Line + Board;
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
		// Somebody is offering. Said at the counter because this is the
		// industry's one presence in the valley -- and a shoe deal is
		// literally free rubber from this shop, so it is where the
		// conversation would actually happen.
		//
		// Only when it beats what you already hold: a shop that offers you
		// the deal you signed last year every time you walk in is a shop
		// nobody reads.
		// The physio, when you are carrying something. Said at the counter
		// because there is no physio *place* yet -- the shop is the town's
		// one desk, and it already handles rubber, pads, dreams and now a
		// sponsor's paperwork. When the town becomes somewhere in its own
		// right this earns its own door.
		// The kit counter: the membership always, the hangboard until you
		// own one. Both are indoor answers to a day the weather has
		// already decided, which is why they sit together.
		FString Kit = FString::Printf(TEXT("\n   %s"),
		                              *Game->MembershipLine());
		const FString Board = Game->HangboardLine();
		if (!Board.IsEmpty())
		{
			Kit += FString::Printf(TEXT("\n   %s"), *Board);
		}
		// And the rack, which is the one purchase that opens a crag rather
		// than improving a day.
		const FString Gear = Game->RackOfferLine();
		if (!Gear.IsEmpty())
		{
			Kit += FString::Printf(TEXT("\n   %s"), *Gear);
		}

		FString Care;
		const FString Physio = Game->PhysioLine();
		if (!Physio.IsEmpty())
		{
			Care = FString::Printf(TEXT("\n   %s"), *Physio);
		}

		// **The counter where an injury stops being a wait.** Everything
		// here is a decision with a wrong answer, and the first of them is
		// whether to pay to find out what is wrong at all -- see
		// Sim/DirtbagMedical.h.
		const FString What = Game->MedicalLine();
		if (!What.IsEmpty())
		{
			Care += FString::Printf(TEXT("\n   %s"), *What);
			if (Game->Player.Medical.Diagnosis != EDirtbagDiagnosis::Scanned)
			{
				Care += FString::Printf(
				    TEXT("\n   Have it looked at?  (V)  $%.0f"),
				    Game->Player.Medical.Diagnosis ==
				            EDirtbagDiagnosis::None
				        ? Game->PriceOfLook(false)
				        : Game->PriceOfLook(true));
			}
			// The shot, and it is the tempting wrong answer: it works this
			// week and marks the joint for the rest of the career.
			if (Game->Player.Medical.Treatment == EDirtbagTreatment::Rest)
			{
				Care += FString::Printf(
				    TEXT("\n   A shot in it?  (C)  $%.0f  -  works now"),
				    Game->PriceOf(EDirtbagTreatment::Cortisone));
				if (Game->Player.Medical.Diagnosis ==
				    EDirtbagDiagnosis::Scanned)
				{
					Care += FString::Printf(
					    TEXT("\n   Operate?  (O)  $%.0f  -  most of a "
					         "season"),
					    Game->PriceOf(EDirtbagTreatment::Surgery));
				}
			}
			// **The decision the whole phase is about.** Whether the stage
			// is done is only shown when you have paid to know.
			Care += Game->ComebackStageIsDone() &&
			                Game->Player.Medical.Diagnosis !=
			                    EDirtbagDiagnosis::None
			            ? TEXT("\n   Move on?  (N)")
			            : TEXT("\n   Push on anyway?  (N)");
		}
		// **The two that are not the injury.** They sit at the same
		// counter because they are the same kind of decision -- spending
		// on yourself rather than on climbing -- and they are the two
		// this game has never asked you to make.
		const FString Ill = Game->SickLine();
		if (!Ill.IsEmpty())
		{
			Care += FString::Printf(TEXT("\n   %s"), *Ill);
			if (!Game->Player.Sickness.bMedicated)
			{
				Care += TEXT("  (K)");
			}
		}
		const FString Tooth = Game->TeethLine();
		if (!Tooth.IsEmpty())
		{
			// It only ever goes one way, and the price on the prompt is
			// today's price, which is the cheapest it will ever be.
			Care += FString::Printf(TEXT("\n   %s  (Y)"), *Tooth);
		}
		if (Game->Player.Cash >= 110.0)
		{
			Care += TEXT("\n   Talk to somebody?  (Z)  $110");
		}

		const FString Cover = Game->InsuranceLine();
		if (!Cover.IsEmpty())
		{
			Care += FString::Printf(TEXT("\n   %s  (B)"), *Cover);
		}
		const FString History = Game->BodyHistoryLine();
		if (!History.IsEmpty())
		{
			Care += FString::Printf(TEXT("\n   %s"), *History);
		}

		const EDirtbagSponsorTier Offer = Game->OfferOnTheTable();
		FString Deal;
		if (Offer > Game->Player.Sponsor.Tier)
		{
			Deal = FString::Printf(
			    TEXT("\n   Somebody has been asking about you.  %s  (S)"),
			    *Game->WhatTheyAreOffering());
		}

		const double Grades = Game->ShoeCostInGrades();
		return (Grades >= 0.05
		    ? FString::Printf(
		          TEXT("Shoes?  (E)  -  %s, costing you %.1f of a grade.  $%.0f"),
		          *Game->ShoeLine(), Grades, Game->Player.Cash)
		    : FString::Printf(TEXT("Shoes?  (E)  -  %s.  $%.0f"),
		                      *Game->ShoeLine(), Game->Player.Cash)) +
		       Kit + Care + Deal;
	}
	case EDirtbagSpotKind::Bivy:
	{
		FString Line = FString::Printf(
		    TEXT("Tonight: %s"),
		    *Game->SpotName(Game->Player.Bivy.Tonight));
		for (int32 i = 0; i < 5; i++)
		{
			const EDirtbagSpot S = static_cast<EDirtbagSpot>(i);
			const FString Why = Game->SpotWhyNot(S);
			Line += FString::Printf(
			    TEXT("\n   %d  %-22s %s"), i + 1, *Game->SpotName(S),
			    Why.IsEmpty() ? *Game->SpotBlurb(S) : *Why);
		}
		const FString Owed = Game->BivyWarningLine();
		if (!Owed.IsEmpty())
		{
			Line += FString::Printf(TEXT("\n   %s  -  pay $%.0f (E)"), *Owed,
			                        Game->WhatTheCityIsOwed());
		}
		return Line;
	}
	case EDirtbagSpotKind::Keeping:
	{
		const FString Smell = Game->GrimeWord();
		switch (Keeps)
		{
		case EDirtbagKeepService::WashInTheVan:
			return FString::Printf(
			    TEXT("Wash?  (E)  -  a rag and a jug.  You are %s"), *Smell);
		case EDirtbagKeepService::TruckStop:
			return FString::Printf(
			    TEXT("Shower?  (E)  -  $8, an hour.  You are %s"), *Smell);
		case EDirtbagKeepService::Lake:
			return FString::Printf(
			    TEXT("Swim?  (E)  -  free, an afternoon.  You are %s"), *Smell);
		case EDirtbagKeepService::Water:
			return FString::Printf(
			    TEXT("Fill the jugs?  (E)  -  $2.  %.0f litres left%s"),
			    Game->Player.Living.Water,
			    Game->CanCook() ? TEXT("") : TEXT(", and you cannot cook"));
		default:
			return FString::Printf(
			    TEXT("Swap the bottle?  (E)  -  $18.  %.0f%% left"),
			    Game->Player.Living.Propane);
		}
	}
	case EDirtbagSpotKind::Gym:
	{
		if (!Game->Player.Gym.bOwned)
		{
			return FString::Printf(
			    TEXT("Buy the gym?  (E)  -  $%.0f.  You have $%.0f"),
			    Game->GymPrice(), Game->Player.Cash);
		}

		// **An open incident takes the whole prompt over**, because while it
		// is open the number keys mean something else entirely, and a
		// prompt that lies about what a key does is worse than no prompt.
		if (Game->Player.Gym.Incident != EDirtbagGymIncident::None)
		{
			return FString::Printf(TEXT("%s\n   %s\n   (1) %s\n   (2) %s"),
			                       *Game->GymLine(), *Game->GymIncidentText(),
			                       *Game->GymIncidentChoiceLine(0),
			                       *Game->GymIncidentChoiceLine(1));
		}
		FString Line = FString::Printf(TEXT("%s\n   %s\n   %s"),
		                               *Game->GymLine(), *Game->GymLeverLine(),
		                               *GymLeverPageName());
		if (GymPage == 0)
		{
			// Who is due a moment, and what went up on the walls this week.
			Line += FString::Printf(TEXT("\n   %s"), *Game->GymFloorLine());
			const FString NotTonight = Game->GymCompWhyNot();
			if (!NotTonight.IsEmpty())
			{
				Line += FString::Printf(TEXT("\n   %s"), *NotTonight);
			}
		}
		else if (GymPage == 2)
		{
			// The shortlist is the page, so it belongs on the page rather
			// than behind a keypress that spends money to read it.
			bool bSeat = true;
			if (WhichSeatIsOpen(Game->Player.Gym, bSeat))
			{
				const TArray<FDirtbagGymStaffer> Pool =
				    Game->GymCandidatesFor(bSeat);
				for (int32 i = 0; i < Pool.Num(); i++)
				{
					Line += FString::Printf(TEXT("\n   (%d) %s, $%d - %s"), i + 1,
					                        *Pool[i].Name,
					                        FMath::RoundToInt(Pool[i].Wage),
					                        *Pool[i].Trait);
				}
			}
			bool bAsking = true;
			if (WhoIsAsking(*Game, bAsking))
			{
				Line += FString::Printf(
				    TEXT("\n   %s wants another $%d a day."),
				    bAsking ? *Game->Player.Gym.Desk.Name
				            : *Game->Player.Gym.RouteSetter.Name,
				    FMath::RoundToInt(Game->GymRaiseAsked(bAsking)));
			}
		}
		else if (GymPage == 6)
		{
			if (Game->Player.GymLeague.bRunning)
			{
				Line += FString::Printf(TEXT("\n   %s"), *Game->GymLeagueLine());
				const TArray<FDirtbagLeagueStanding> Table =
				    Game->TheLeagueTable();
				// **The top three, not all thirteen.** A table on a wall is
				// a table; a table on a HUD is a wall of text.
				for (int32 i = 0; i < Table.Num() && i < 3; i++)
				{
					Line += FString::Printf(
					    TEXT("\n   %d. %s %.1f%s"), i + 1, *Table[i].Name,
					    Table[i].Points,
					    Table[i].bSuited ? TEXT("  (it suits them)") : TEXT(""));
				}
			}
			else
			{
				const FString Cannot = Game->LeagueWhyNotStart();
				if (!Cannot.IsEmpty())
				{
					Line += FString::Printf(TEXT("\n   %s"), *Cannot);
				}
				else
				{
					for (int32 i = 0; i < 4; i++)
					{
						Line += FString::Printf(
						    TEXT("\n   (%d) %s"), i + 1,
						    *Game->LeagueFormatLine(
						        static_cast<EDirtbagLeagueFormat>(i)));
					}
				}
			}
		}
		else if (GymPage == 5)
		{
			Line += FString::Printf(TEXT("\n   %s"), *Game->HostingLine());
			const FString Cannot = Game->BidWhyNot();
			if (!Cannot.IsEmpty() && !Game->HostingThisSeason())
			{
				Line += FString::Printf(TEXT("\n   %s"), *Cannot);
			}
		}
		else if (GymPage == 4)
		{
			// The squad, or the reason there is not one yet.
			const FString Squad = Game->YouthTeamLine();
			Line += FString::Printf(
			    TEXT("\n   %s"),
			    Squad.IsEmpty() ? *Game->YouthWhyNot() : *Squad);
			if (Game->Player.Youth.bGoing)
			{
				const FString NotTonight = Game->YouthSessionWhyNot();
				if (!NotTonight.IsEmpty())
				{
					Line += FString::Printf(TEXT("\n   %s"), *NotTonight);
				}
			}
		}
		else if (GymPage == 3)
		{
			for (int32 i = 0; i < 5; i++)
			{
				Line += FString::Printf(
				    TEXT("\n   (%d) %s"), i + 1,
				    *Game->GymWingShopLine(static_cast<EDirtbagGymWing>(i)));
			}
		}
		const FString Trouble = Game->GymWarningLine();
		if (!Trouble.IsEmpty())
		{
			Line += FString::Printf(TEXT("\n   %s"), *Trouble);
		}
		return Line;
	}
	case EDirtbagSpotKind::Evening:
	{
		// Two different prompts, because taking a thing up and keeping it
		// going are two different decisions. The status line does the work
		// once it is yours -- it is the whole of the warning you get.
		const FString Status = Game->ThreadStatusLine(Thread);
		if (Status.IsEmpty())
		{
			return FString::Printf(TEXT("%s  (E)  -  %s"),
			                       *Game->ThreadDescription(Thread),
			                       *Game->WaitAdvice());
		}
		return FString::Printf(TEXT("%.1f hours?  (E)  -  %s"),
		                       Game->ThreadHours(Thread), *Status);
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
			// Creation only; see OnChoose4.
			InputComponent->BindKey(EKeys::Four, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnChoose4);
			InputComponent->BindKey(EKeys::Five, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnChoose5);
			InputComponent->BindKey(EKeys::Six, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnChoose6);
			// Its own key, nowhere near E. See OnRetire.
			InputComponent->BindKey(EKeys::R, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnRetire);
			InputComponent->BindKey(EKeys::G, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnGuidebook);
			InputComponent->BindKey(EKeys::S, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnSign);
			InputComponent->BindKey(EKeys::P, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnPhysio);
			// The care counter's own keys. Deliberately away from the
			// movement block and away from E, which is every other spot's
			// yes: none of these is a yes, they are all a decision.
			InputComponent->BindKey(EKeys::V, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnLookAtIt);
			InputComponent->BindKey(EKeys::C, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnTakeTheShot);
			InputComponent->BindKey(EKeys::O, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnOperate);
			InputComponent->BindKey(EKeys::N, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnPushOn);
			InputComponent->BindKey(EKeys::B, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnCover);
			InputComponent->BindKey(EKeys::K, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnTakeSomething);
			// Held, not pressed: it modifies the number key that follows.
			InputComponent->BindKey(EKeys::W, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnHardWayDown);
			InputComponent->BindKey(EKeys::W, IE_Released, this,
			                        &ADirtbagDaySpot::OnHardWayUp);
			InputComponent->BindKey(EKeys::Y, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnTooth);
			InputComponent->BindKey(EKeys::Z, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnShrink);
			// At the van, where a morning starts.
			InputComponent->BindKey(EKeys::X, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnPrehab);
			InputComponent->BindKey(EKeys::M, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnMembership);
			InputComponent->BindKey(EKeys::J, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnSalary);
			InputComponent->BindKey(EKeys::H, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnHangboard);
			InputComponent->BindKey(EKeys::G, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnRack);
			// `CLB-32`: an evening with the tape, at the van. Its own key
			// because it is not a yes to anything -- it is a decision to
			// spend a night not sleeping.
			InputComponent->BindKey(EKeys::L, IE_Pressed, this,
			                        &ADirtbagDaySpot::OnStudy);
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
	// A confirm must not sit armed across half a season: walk away and the
	// question is withdrawn.
	bRetireArmed = false;
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

void ADirtbagDaySpot::OnSalary()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Shift)
	{
		return;
	}

	if (Game->Player.Job.bSalaried)
	{
		Game->QuitSalariedJob();
		// The cost is psyche, and it is said rather than shown as a number
		// -- you have just given up the only reliable money in the game
		// and everybody at the fire will have an opinion.
		Say(TEXT("You hand the keys back. It takes a few days to stop "
		         "feeling like a mistake."),
		    FColor::Yellow, 8.f);
		PushPrompt();
		return;
	}

	const int32 Lost = Game->TakeSalariedJob();
	// No ceremony and no warning. The whole design of this trap is that
	// taking it is entirely reasonable -- so the game reports what
	// happened, including the streak it just ended, and says nothing about
	// whether it was wise.
	Say(Lost >= 365
	        ? FString::Printf(
	              TEXT("You start Monday.  That ends %d days nobody owned."),
	              Lost)
	        : FString(TEXT("You start Monday.")),
	    FColor::Yellow, 8.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnMembership()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop)
	{
		return;
	}
	if (!Game->RenewGymMembership())
	{
		Say(FString::Printf(TEXT("$%.0f, and you have $%.0f."),
		                    dirtbag::KitDials{}.membershipCost,
		                    Game->Player.Cash),
		    FColor::Orange, 5.f);
		return;
	}
	Say(FString::Printf(TEXT("Signed up.  %s"), *Game->MembershipLine()),
	    FColor::Green, 6.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnRack()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop)
	{
		return;
	}
	const dirtbag::TradDials Td;
	const dirtbag::RackTier Have =
	    dirtbag::TierOf(DirtbagConvert::ToSim(Game->Player.Rack), Td);
	if (Have == dirtbag::RackTier::Doubles)
	{
		Say(TEXT("There is nothing on this shelf you have not got."),
		    FColor::Orange, 5.f);
		return;
	}
	const dirtbag::RackTier Want =
	    static_cast<dirtbag::RackTier>(static_cast<int>(Have) + 1);
	if (!Game->BuyTradRack())
	{
		Say(FString::Printf(TEXT("$%.0f, and you have $%.0f."),
		                    dirtbag::RackPrice(Want, Td) * Game->ShopPrice(),
		                    Game->Player.Cash),
		    FColor::Orange, 5.f);
		return;
	}
	// The first rack is a different sentence from the fourth cam.
	Say(Have == dirtbag::RackTier::None
	        ? TEXT("It goes over your shoulder on the way out. The buttress "
	               "is an hour up the hill.")
	        : TEXT("Racked up. You will notice it on the thin ones."),
	    FColor::Green, 6.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnHangboard()
{
	if (!bPlayerNear || !Game)
	{
		return;
	}

	// At the counter it is a purchase; at the van it is a session. One
	// object, and where you are standing says which you meant.
	if (Kind == EDirtbagSpotKind::GearShop)
	{
		if (Game->Player.Kit.bHangboard)
		{
			return;
		}
		if (!Game->BuyHangboard())
		{
			Say(FString::Printf(TEXT("$%.0f, and you have $%.0f."),
			                    dirtbag::KitDials{}.hangboardCost,
			                    Game->Player.Cash),
			    FColor::Orange, 5.f);
			return;
		}
		Say(TEXT("It goes in the van, over the side door."), FColor::Green,
		    6.f);
		PushPrompt();
		return;
	}

	if (Kind != EDirtbagSpotKind::Van || !Game->Player.Kit.bHangboard)
	{
		return;
	}
	if (!Game->HangboardSession())
	{
		// The two refusals the sim has, said as themselves: once a day, and
		// not on skin that is already gone.
		Say(Game->Day.bHangboardDone
		        ? TEXT("You have already hung today.")
		        : TEXT("Not on this skin. That is how you take a week off."),
		    FColor::Orange, 5.f);
		return;
	}
	Say(FString::Printf(TEXT("Twenty minutes on the board.  It's %.0f:00, "
	                         "energy %.0f."),
	                    Game->Day.Hour, Game->Day.Energy),
	    FColor::Green, 6.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnHardWayDown() { bHoldingTheHardWay = true; }
void ADirtbagDaySpot::OnHardWayUp() { bHoldingTheHardWay = false; }

void ADirtbagDaySpot::OnTakeSomething()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (!Game->TakeSomethingForIt())
	{
		return;   // not ill, already took something, or eleven dollars
	}
	Say(Game->SickLine(), FColor::Green, 6.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnTooth()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (Game->Player.Teeth.Stage == EDirtbagToothStage::Fine) return;
	if (!Game->FixTheTooth())
	{
		// **It only ever goes one way**, so a refusal is worth saying with
		// the number in it: the price on this prompt is the cheapest it
		// will ever be.
		Say(FString::Printf(
		        TEXT("$%.0f, and you have $%.0f.  It will be more than "
		             "that later."),
		        Game->ToothPrice(), Game->Player.Cash),
		    FColor::Orange, 7.f);
		return;
	}
	Say(Game->MedicalNews, FColor::Green, 7.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnShrink()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (!Game->SeeTheShrink())
	{
		Say(TEXT("Not this week, or not for that money."), FColor::Orange,
		    5.f);
		return;
	}
	Say(Game->MedicalNews, FColor::Green, 7.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnPrehab()
{
	// At the van, because it is twenty minutes of a morning and a morning
	// starts where you slept.
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Van) return;
	if (!Game->DoPrehab())
	{
		Say(TEXT("Already did it."), FColor::Silver, 3.f);
		return;
	}
	// **Boring, it works, and nobody does it.** The line says what the
	// streak is worth precisely because nothing else about it is visible.
	const FString Worth = Game->UpkeepLine();
	Say(Worth.IsEmpty()
	        ? FString(TEXT("Twenty minutes on the floor by the van."))
	        : FString::Printf(TEXT("Twenty minutes on the floor.  %s"),
	                          *Worth),
	    FColor::Silver, 6.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnLookAtIt()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (!Game->IsHurt()) return;
	// **One key, escalating.** Nobody wants a menu for "find out what is
	// wrong"; the first press buys a pair of hands and the second buys the
	// machine, which is the order anybody actually does it in.
	const bool bOk = Game->Player.Medical.Diagnosis == EDirtbagDiagnosis::None
	                     ? Game->SeeSomebody()
	                     : Game->GetItScanned();
	Say(bOk ? Game->MedicalNews
	        : TEXT("You cannot cover it, and it is not going to look at "
	               "itself."),
	    bOk ? FColor::Green : FColor::Orange, 7.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnTakeTheShot()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (!Game->IsHurt()) return;
	if (!Game->TakeTheShot())
	{
		Say(TEXT("Not for this one, and not twice."), FColor::Orange, 5.f);
		return;
	}
	Say(Game->MedicalNews, FColor::Yellow, 8.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnOperate()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (!Game->IsHurt()) return;
	if (!Game->BookTheSurgery())
	{
		// The three reasons, said rather than guessed at: nobody operates
		// on a guess, nobody operates on a strain, and nobody operates on
		// credit.
		Say(Game->Player.Medical.Diagnosis != EDirtbagDiagnosis::Scanned
		        ? TEXT("Nobody is operating on a guess.")
		        : TEXT("Not for this, or not for that money."),
		    FColor::Orange, 6.f);
		return;
	}
	Say(Game->MedicalNews, FColor::Yellow, 9.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnPushOn()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (!Game->IsHurt()) return;
	const bool bFine = Game->PushOn();
	Say(Game->MedicalNews, bFine ? FColor::Green : FColor::Red, 8.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnCover()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop) return;
	if (Game->Player.Medical.bInsured)
	{
		Game->CancelInsurance();
		Say(TEXT("Cancelled. You are on your own now."), FColor::Silver, 6.f);
	}
	else if (Game->BuyInsurance())
	{
		Say(Game->MedicalNews, FColor::Green, 6.f);
	}
	else
	{
		// Everybody tries this.
		Say(TEXT("They will not write a policy on something that is "
		         "already wrong."),
		    FColor::Orange, 6.f);
	}
	PushPrompt();
}

void ADirtbagDaySpot::OnPhysio()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop)
	{
		return;
	}
	if (!Game->IsHurt())
	{
		return;
	}
	if (!Game->SeeAPhysio())
	{
		// The reason, not a failure. PhysioLine already knows which of the
		// three it is -- too soon, too poor, or fine -- so it says it
		// rather than this branch guessing again.
		Say(Game->PhysioLine(), FColor::Orange, 6.f);
		return;
	}
	Say(FString::Printf(TEXT("An hour of somebody digging their thumb into "
	                         "it.  %s  $%.0f left."),
	                    *Game->InjuryLine(), Game->Player.Cash),
	    FColor::Green, 8.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnSign()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::GearShop)
	{
		return;
	}
	const FString What = Game->WhatTheyAreOffering();
	if (!Game->SignWithSponsor())
	{
		return;
	}
	// Said once, flatly, with what it will cost attached -- the game does
	// not congratulate you for this any more than it congratulates you for
	// a shortcut. You have swapped days for money and the days are the
	// expensive half.
	Say(FString::Printf(TEXT("Signed.  %s"), *What), FColor::Yellow, 9.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnStudy()
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Van)
	{
		return;
	}
	// **Gated on its own reason.** The sim says why not, and it says why
	// not in the game's voice -- so the refusal is the same sentence
	// wherever it is asked.
	const FString Why = Game->ScoutWhyNot();
	if (!Game->ScoutTheField())
	{
		if (!Why.IsEmpty()) { Say(Why, FColor::Silver, 5.f); }
		return;
	}
	Say(TEXT("An evening with the tape. You know what they do on a slab "
	         "now, and what they do when it gets thin."),
	    FColor::Yellow, 7.f);
	PushPrompt();
}

void ADirtbagDaySpot::OnGuidebook()
{
	// Readable anywhere you can press a key, including mid-evening at the
	// fire -- deciding what to get on tomorrow is exactly the thing you do
	// while somebody else is dealing.
	if (!Game || !bPlayerNear)
	{
		return;
	}
	// Asked-to-open-and-still-shut means something refused it, and a key
	// that does nothing silently reads as a key that is broken. Detected
	// this way rather than by re-testing the venue here, so the rule about
	// where there is a book lives in exactly one place.
	const bool bWas = Game->Guidebook.bActive;
	const bool bNow = Game->ToggleGuidebook();
	if (!bWas && !bNow)
	{
		Say(TEXT("Plastic. The setter's tag is the whole of the book here."),
		    FColor::Silver, 4.f);
	}
	PushPrompt();
}

bool ADirtbagDaySpot::TurnGuidebookPage(int32 Which)
{
	if (!Game || !Game->Guidebook.bActive)
	{
		return false;
	}
	Game->SetGuidebookView(static_cast<EDirtbagGuidebookView>(
	    FMath::Clamp(Which, 0, 2)));
	return true;
}

void ADirtbagDaySpot::OnRetire()
{
	if (!Game)
	{
		return;
	}
	// While the handover is up, R advances it -- so the same key that
	// started the thing carries it through, and there is never a screen
	// whose only exit is a key nobody mentioned.
	if (Game->Handover.bActive)
	{
		Game->StepHandover();
		PushPrompt();
		return;
	}
	if (!bPlayerNear || Kind != EDirtbagSpotKind::Van)
	{
		return;
	}

	// A climber nobody has named gets named, and that is all R does until
	// they are. No confirm: putting a name to yourself is not irreversible
	// the way stopping is, and a fresh career pressing R almost certainly
	// means "who am I" rather than "I am done".
	if (Game->ClimberName.IsEmpty())
	{
		FDirtbagHandoverReadout& H = Game->Handover;
		H = FDirtbagHandoverReadout();
		H.bActive = true;
		H.Step = EDirtbagHandoverStep::Choosing;
		H.Candidates = Game->WhoCouldTurnUp();
		H.Generation = Game->GenerationsBefore();
		// No epitaph: nothing has ended. The screen reads that as a
		// naming rather than a handover and draws itself accordingly.
		PushPrompt();
		return;
	}

	if (!bRetireArmed)
	{
		bRetireArmed = true;
		// The game does not talk you into it. TimeToThinkAboutIt is the
		// only opinion it ever offers and it is never a command; this
		// prompt reports the state of the body and nothing else.
		Say(Game->TimeToThinkAboutIt()
		        ? TEXT("Stop climbing, for good?  R again to mean it.")
		        : TEXT("Stop climbing, for good?  You have years in you.  "
		               "R again to mean it."),
		    FColor::Yellow, 8.f);
		PushPrompt();
		return;
	}

	bRetireArmed = false;

	// The epitaph is read off the career that is still running, so it must
	// be taken *before* the handover tallies and replaces it.
	FDirtbagHandoverReadout& H = Game->Handover;
	H = FDirtbagHandoverReadout();
	H.bActive = true;
	H.Step = EDirtbagHandoverStep::Epitaph;
	H.Epitaph = Game->CareerEpitaph();
	H.Generation = Game->GenerationsBefore();
	// Deliberately no guidebook yet. `InheritedGuidebook` walks the
	// *filed* careers, so asking now would list your predecessors' lines
	// under your own epitaph -- which is exactly backwards, and was the
	// first version of this. It is asked after the handover, where it
	// means what its name says: the book the next climber opens, with the
	// career that just ended newly in it.
	PushPrompt();
}

void ADirtbagDaySpot::OnInteract()
{
	if (SkipTravel())
	{
		return;
	}
	// Creation and the handover own E while either is up, the same way the
	// road owns every key: there is nothing else to interact with from
	// inside them. The decision lives on the game instance because this
	// spot is not the only thing listening -- see
	// ADirtbagUEPlayerController, which is what serves these screens when
	// the player is not standing at anything at all.
	if (Game && Game->PressOnAScreen())
	{
		PushPrompt();
		return;
	}
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
		// Deliberately not "take the best one". The board's whole point is
		// that the gigs are not interchangeable -- the best-paying one
		// costs you the old guard and the stewards both -- so a key that
		// silently picked would hand somebody a standing hit they never
		// chose. E says where to look; the numbers do the work.
		Say(TEXT("It's all on the board.  1, 2 or 3."), FColor::Cyan, 5.f);
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
	case EDirtbagSpotKind::Bivy:
	{
		// E clears what the city is owed; where you park is on the numbers.
		Say(Game->PayTheTickets()
		        ? FString(TEXT("Paid. The van is yours again."))
		        : FString(TEXT("Nothing owed, or not enough to clear it.")),
		    FColor::Yellow, 5.f);
		break;
	}
	case EDirtbagSpotKind::Keeping:
	{
		bool bDid = false;
		switch (Keeps)
		{
		case EDirtbagKeepService::WashInTheVan: bDid = Game->WashInTheVan(); break;
		case EDirtbagKeepService::TruckStop: bDid = Game->ShowerAtTheTruckStop(); break;
		case EDirtbagKeepService::Lake: Game->SwimInTheLake(); bDid = true; break;
		case EDirtbagKeepService::Water: bDid = Game->FillTheJugs(); break;
		default: bDid = Game->SwapTheBottle(); break;
		}
		Say(bDid ? FString::Printf(TEXT("You are %s."), *Game->GrimeWord())
		         : FString(TEXT("Not for that, or there is nothing to do.")),
		    bDid ? FColor::Yellow : FColor::Silver, 5.f);
		break;
	}
	case EDirtbagSpotKind::Gym:
	{
		if (!Game->Player.Gym.bOwned)
		{
			Say(Game->BuyTheGym(GymNameToBuy)
			        ? FString::Printf(TEXT("%s is yours."), *GymNameToBuy)
			        : FString(TEXT("Not for that.")),
			    FColor::Yellow, 6.f);
			break;
		}
		// **Pressing E on a gym you already own runs the ads.** The five
		// levers above are states you set; a campaign is a thing you do,
		// so it belongs on the verb key rather than on a number.
		if (Game->LaunchGymCampaign(EDirtbagGymCampaign::Social) ||
		    Game->LaunchGymCampaign(EDirtbagGymCampaign::Flyers))
		{
			Say(TEXT("The ads are out."), FColor::Yellow, 5.f);
			break;
		}
		// Nothing to launch, so it reads the room instead. **The town is
		// what moved the membership while you were not looking**, and this
		// is the only place it gets said out loud.
		Say(Game->TheTownLine(), FColor::Silver, 9.f);
		break;
	}
	case EDirtbagSpotKind::Evening:
	{
		// **One press, whether or not it is yours yet.** The sim takes it
		// up on the first go and charges the same hours for it -- see
		// Sim/DirtbagDay.h, where a free first date turned out to be worth
		// a hundred and ninety free relationships in a thirty-year career.
		const bool bWasNew = Game->ThreadStatusLine(Thread).IsEmpty();
		const double Took = Game->ThreadHours(Thread);
		if (!Game->SpendTheEvening(Thread))
		{
			Say(TEXT("Not yet."), FColor::Silver, 4.f);
			break;
		}
		Say(bWasNew ? Game->ThreadDescription(Thread)
		            : FString::Printf(TEXT("%.1f hours.  %s"), Took,
		                              *Game->ThreadStatusLine(Thread)),
		    FColor::Yellow, 6.f);
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
		// `TAX-1`: the reckoning happened inside the night tick, so this is
		// where you find out -- and the warning is here too, because three
		// days out at the van is where you can still do something about it.
		// A career that has never won anything sees neither, which is most
		// of them.
		Say(Game->BillLine(), FColor::Silver, 7.f);
		Say(Game->TaxNewsLine(), FColor::Orange, 9.f);
		Say(Game->TaxWarningLine(), FColor::Silver, 7.f);
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

bool ADirtbagDaySpot::TakeGig(int32 Which)
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Shift)
	{
		return false;
	}
	const TArray<FDirtbagOddJob> Board = Game->TodaysJobBoard();
	if (!Board.IsValidIndex(Which))
	{
		return true;
	}
	const FDirtbagOddJob& Gig = Board[Which];

	// **Held W means do it properly.** A modifier rather than a second key
	// per gig, because the decision belongs to the gig you are picking and
	// not to a menu of its own -- and because the easy answer has to be the
	// one you get by just pressing the number, which is what a tired person
	// does.
	if (bHoldingTheHardWay)
	{
		Game->TakeTheGigTheHardWay();
	}

	if (!Game->TakeOddJob(Gig))
	{
		// The only reason it refuses. Said as the reason rather than as a
		// failure, because the van being dead is a thing the player is
		// already living with and this is one more place it bites: the
		// breakdown costs you the fix *and* the work that would have paid
		// for it.
		Say(FString::Printf(TEXT("%s needs the van, and the van is off the "
		                         "road."),
		                    *Gig.Name),
		    FColor::Orange, 6.f);
		PushPrompt();
		return true;
	}

	Say(FString::Printf(TEXT("%s.  +$%.0f, %.0f hours gone.  It's %.0f:00."),
	                    *Gig.Name, Gig.Pay, Gig.Hours, Game->Day.Hour),
	    FColor::Green, 6.f);
	// What the gig did to the dog, if it did anything. Said separately
	// because it is not part of the transaction.
	if (!Game->DogWorry.IsEmpty())
	{
		Say(Game->DogWorry, FColor::Orange, 6.f);
	}
	PushPrompt();
	return true;
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
	// A prompt may carry more than one line -- the van says what it needs
	// and then, separately, that you have been thinking about stopping.
	// Split here rather than embedding newlines in the drawn string,
	// because the canvas draws a string as one line however many newlines
	// are in it, and the second half would come out as a box.
	TArray<FString> Parts;
	Line.Text.ParseIntoArray(Parts, TEXT("\n"), true);
	TArray<FDirtbagPromptLine> Lines;
	for (const FString& Part : Parts)
	{
		if (Part.IsEmpty())
		{
			continue;
		}
		FDirtbagPromptLine One;
		One.Text = Part;
		One.Tone = Line.Tone;
		Lines.Add(One);
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

// **The rival's offer answers on the same two keys as everything else**, and
// only at the van. C/F belong to the card table at the fire, and a key that
// means "call the lie" in one trigger and "take a partner for life" in the
// next is how somebody agrees to rope up trying to fold a hand.
bool ADirtbagDaySpot::AnswerTheRival(bool bYes)
{
	if (!bPlayerNear || !Game || Kind != EDirtbagSpotKind::Sleep)
	{
		return false;
	}
	if (Game->RivalOffer.IsEmpty())
	{
		return false;
	}
	if (bYes ? Game->AcceptTheRival() : Game->DeclineTheRival())
	{
		Say(Game->RivalNews, FColor::Yellow, 8.f);
		PushPrompt();
		return true;
	}
	return false;
}

void ADirtbagDaySpot::OnCommit()
{
	if (SkipTravel())
	{
		return;
	}
	if (AnswerTheRival(true))
	{
		return;
	}
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
	if (SkipTravel())
	{
		return;
	}
	if (AnswerTheRival(false))
	{
		return;
	}
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
// The screens are asked first and from the game instance -- see
// UDirtbagGameInstance::ChooseOnAScreen. A full-screen screen outranks
// whatever the player happens to be standing in front of, which used to be
// the other way round: creation was asked third, after a bivy and a gym
// counter had each had a look at the key.
bool ADirtbagDaySpot::PickABivy(int32 Index)
{
	if (!Game || !bPlayerNear || Kind != EDirtbagSpotKind::Bivy) { return false; }
	if (Index < 0 || Index >= 5) { return false; }
	const EDirtbagSpot Where = static_cast<EDirtbagSpot>(Index);
	if (!Game->ParkAt(Where))
	{
		Say(Game->SpotWhyNot(Where), FColor::Silver, 5.f);
		return true;
	}
	Say(FString::Printf(TEXT("Tonight: %s"), *Game->SpotName(Where)),
	    FColor::Yellow, 5.f);
	return true;
}

FString ADirtbagDaySpot::GymLeverPageName() const
{
	switch (GymPage)
	{
	// **The floor is page one**, because it is the thing you do when you
	// walk in and the only part of owning a gym that is about people. The
	// books run whether you are here or not.
	case 0: return TEXT("the floor:  walk it (1)   comp night (2)   more (6)");
	case 1: return TEXT("the levers:  price (1/2/3)   sets (4)   kit (5)   more (6)");
	case 2: return TEXT("the people:  hire (1/2/3)   raise: yes (4) no (5)   more (6)");
	case 3: return TEXT("the building:  wings (1-5)   more (6)");
	case 4: return TEXT("the squad:  found it / session (1)   coach: you (2) hired (3)   more (6)");
	case 5: return TEXT("the federation:  bid for the season (1)   run the round (2)   more (6)");
	case 6: return TEXT("league night:  start one (1-4)   run tonight (5)   more (6)");
	default: return TEXT("the keys:  hand it over (1)   the town (2)   more (6)");
	}
}

// And who is asking for more money, in the same order.
bool ADirtbagDaySpot::PullGymLever(int32 Index)
{
	if (!Game || !bPlayerNear || Kind != EDirtbagSpotKind::Gym) { return false; }
	if (!Game->Player.Gym.bOwned) { return false; }

	// **The clipboard outranks the levers.** While something is open the
	// first two keys answer it and nothing else is on offer, because it is
	// the one thing here with a clock on it -- leave it four days and it
	// answers itself, the cheap way.
	if (Game->Player.Gym.Incident != EDirtbagGymIncident::None)
	{
		if (Index > 1) { return false; }
		Say(Game->AnswerTheGymIncident(Index)
		        ? Game->Player.GymNews
		        : FString(TEXT("Not for that.")),
		    FColor::Yellow, 8.f);
		return true;
	}

	// Six keys, and pass two put more than six verbs behind this counter.
	if (Index == 5)
	{
		GymPage = (GymPage + 1) % 8;
		Say(GymLeverPageName(), FColor::Silver, 4.f);
		return true;
	}

	bool bSeat = true;
	switch (GymPage)
	{
	case 0:
		if (Index == 0)
		{
			// An hour among your members, once a day. The line it comes
			// back with is the whole reason to be standing here.
			Say(Game->WalkTheFloor()
			        ? Game->Player.GymNews
			        : FString(TEXT("You already walked the floor today - let "
			                       "the members breathe.")),
			    FColor::Yellow, 10.f);
			return true;
		}
		if (Index == 1)
		{
			if (Game->HostACompNight())
			{
				Say(FString::Printf(TEXT("Comp night at %s. %s"),
				                    *Game->Player.Gym.Name,
				                    *Game->Player.GymNews),
				    FColor::Yellow, 10.f);
				return true;
			}
			Say(Game->GymCompWhyNot(), FColor::Silver, 7.f);
			return true;
		}
		return true;

	case 1:
		switch (Index)
		{
		case 0: Game->SetGymPrice(EDirtbagGymPrice::Budget); break;
		case 1: Game->SetGymPrice(EDirtbagGymPrice::Standard); break;
		case 2: Game->SetGymPrice(EDirtbagGymPrice::Premium); break;
		case 3:
		{
			// Cycles, because three mixes on one key is a cycle and three
			// more keys for a lever nobody pulls twice a season is not.
			const uint8 Next = (static_cast<uint8>(Game->Player.Gym.Mix) + 1) % 3;
			Game->SetGymMix(static_cast<EDirtbagGymSetMix>(Next));
			break;
		}
		default:
			Say(Game->UpgradeGymEquipment()
			        ? TEXT("The kit is in.")
			        : TEXT("Not for that, or there is nothing left to buy."),
			    FColor::Yellow, 5.f);
			return true;
		}
		break;

	case 2:
		if (Index <= 2)
		{
			if (!WhichSeatIsOpen(Game->Player.Gym, bSeat))
			{
				Say(TEXT("Both jobs are taken."), FColor::Silver, 4.f);
				return true;
			}
			const TArray<FDirtbagGymStaffer> Pool = Game->GymCandidatesFor(bSeat);
			if (!Pool.IsValidIndex(Index)) { return true; }
			const FDirtbagGymStaffer Who = Pool[Index];
			Say(Game->HireForTheGym(bSeat, Index)
			        ? FString::Printf(TEXT("%s starts tomorrow - $%d a day. %s"),
			                          *Who.Name, FMath::RoundToInt(Who.Wage),
			                          *Who.Trait)
			        : FString(TEXT("Not for that.")),
			    FColor::Yellow, 7.f);
			return true;
		}
		{
			if (!WhoIsAsking(*Game, bSeat))
			{
				Say(TEXT("Nobody is asking."), FColor::Silver, 4.f);
				return true;
			}
			const FString Said = Game->AnswerTheGymRaise(bSeat, Index == 3);
			Say(Said.IsEmpty() ? FString(TEXT("Nobody is asking.")) : Said,
			    FColor::Yellow, 7.f);
		}
		return true;

	case 3:
	{
		const EDirtbagGymWing Wing = static_cast<EDirtbagGymWing>(Index);
		Say(Game->BuildGymWing(Wing)
		        ? FString::Printf(TEXT("%s - up and open."),
		                          *Game->GymWingShopLine(Wing))
		        : Game->GymWingShopLine(Wing),
		    FColor::Yellow, 7.f);
		return true;
	}

	case 4:
		// **One key, because founding it and running it are the same
		// intention** -- you press the squad key and the squad happens,
		// whichever of the two it is tonight.
		if (Index == 0)
		{
			if (!Game->Player.Youth.bGoing)
			{
				if (Game->FoundTheYouthTeam())
				{
					Say(FString::Printf(
					        TEXT("%s has a youth team. %s - and a lot of "
					             "paperwork."),
					        *Game->Player.Gym.Name, *Game->YouthTeamLine()),
					    FColor::Yellow, 10.f);
					return true;
				}
				Say(Game->YouthWhyNot(), FColor::Silver, 7.f);
				return true;
			}
			Say(Game->RunAYouthSession() ? Game->Player.GymNews
			                             : Game->YouthSessionWhyNot(),
			    FColor::Yellow, 10.f);
			return true;
		}
		if (Index == 1 || Index == 2)
		{
			const bool bHired = Index == 2;
			if (!Game->SetTheYouthCoach(bHired))
			{
				Say(Game->Player.Youth.bGoing
				        ? TEXT("Already that way round.")
				        : TEXT("There is no squad yet."),
				    FColor::Silver, 4.f);
				return true;
			}
			Say(bHired
			        ? FString::Printf(
			              TEXT("%s takes the squad - $30 a day on the gym's "
			                   "books. Steady hands. Not you."),
			              *Game->Player.Youth.CoachName)
			        : FString(TEXT("You take the sessions back. It is your "
			                       "evening again, and theirs.")),
			    FColor::Yellow, 8.f);
			return true;
		}
		return true;

	case 5:
		if (Index == 0)
		{
			const FString Cannot = Game->BidWhyNot();
			if (!Cannot.IsEmpty())
			{
				Say(Cannot, FColor::Silver, 7.f);
				return true;
			}
			// **Answered on the spot, and the deposit is gone either way.**
			Say(Game->BidToHostTheSeason()
			        ? FString::Printf(
			              TEXT("%s gets the season. The circuit is coming to "
			                   "your building - and on the day you will have "
			                   "to pick a side of the clipboard."),
			              *Game->Player.Gym.Name)
			        : FString(TEXT("They went elsewhere. The deposit is not "
			                       "coming back.")),
			    FColor::Yellow, 10.f);
			return true;
		}
		if (Index == 1)
		{
			Say(Game->RunTheCircuitRound() ? Game->Player.GymNews
			                               : Game->RoundWhyNot(),
			    FColor::Yellow, 10.f);
			return true;
		}
		return true;

	case 6:
		if (Index <= 3 && !Game->Player.GymLeague.bRunning)
		{
			const EDirtbagLeagueFormat Format =
			    static_cast<EDirtbagLeagueFormat>(Index);
			// **The night comes off the day you start it**, which is the
			// smallest honest answer: you announce it on a Wednesday and it
			// runs on Wednesdays.
			if (Game->StartTheLeague(Format, Game->LeagueNightFromToday()))
			{
				Say(Game->GymLeagueLine(), FColor::Yellow, 10.f);
				return true;
			}
			Say(Game->LeagueWhyNotStart(), FColor::Silver, 7.f);
			return true;
		}
		if (Index == 4 || (Index <= 3 && Game->Player.GymLeague.bRunning))
		{
			Say(Game->RunTheLeagueNight() ? Game->Player.GymNews
			                              : Game->LeagueWhyNotTonight(),
			    FColor::Yellow, 10.f);
			return true;
		}
		return true;

	default:
		if (Index == 0)
		{
			const bool bWas = Game->Player.Gym.bPassive;
			if (!Game->SetGymHandsOff(!bWas))
			{
				Say(TEXT("Needs a front desk and a setter before it can run "
				         "without you."),
				    FColor::Silver, 6.f);
				return true;
			}
			Say(bWas ? TEXT("Back in the books - you are calling the shots "
			                "again.")
			         : TEXT("It runs itself now. Pricing, mix and marketing "
			                "lock in as they are."),
			    FColor::Yellow, 7.f);
			return true;
		}
		if (Index == 1)
		{
			Say(Game->TheTownLine(), FColor::Silver, 9.f);
			return true;
		}
		return true;
	}
	Say(Game->GymLine(), FColor::Yellow, 5.f);
	return true;
}

void ADirtbagDaySpot::OnChoose1()
{
	if (Game && Game->ChooseOnAScreen(0)) { return; }
	if (SkipTravel()) { return; }
	if (PickABivy(0)) { return; }
	if (PullGymLever(0)) { return; }
	if (TurnGuidebookPage(0)) { return; }
	if (TakeGig(0)) { return; }
	if (!SetStakeNotch(0)) { ChooseDreamAt(EDirtbagDream::Rig); }
}
void ADirtbagDaySpot::OnChoose2()
{
	if (Game && Game->ChooseOnAScreen(1)) { return; }
	if (SkipTravel()) { return; }
	if (PickABivy(1)) { return; }
	if (PullGymLever(1)) { return; }
	if (TurnGuidebookPage(1)) { return; }
	if (TakeGig(1)) { return; }
	if (!SetStakeNotch(1)) { ChooseDreamAt(EDirtbagDream::WarChest); }
}
void ADirtbagDaySpot::OnChoose3()
{
	if (Game && Game->ChooseOnAScreen(2)) { return; }
	if (SkipTravel()) { return; }
	if (PickABivy(2)) { return; }
	if (PullGymLever(2)) { return; }
	if (TurnGuidebookPage(2)) { return; }
	if (TakeGig(2)) { return; }
	if (!SetStakeNotch(2)) { ChooseDreamAt(EDirtbagDream::HomeBase); }
}

// **Three more keys, and they exist for the questions that need six.** Six
// origins need six keys, and a question that offers a sixth option with no
// way to press it is the bug the wall's ethics prompt had a day ago. Past
// the screens they reach only the bivy list and the gym counter, both of
// which have six of something; they deliberately do not reach the shop or
// the fire, because a number key that quietly means something at the shop
// and something else at the fire is how a player buys a dream trying to
// fold a hand.
void ADirtbagDaySpot::OnChoose4()
{
	if (Game && Game->ChooseOnAScreen(3)) { return; }
	if (PickABivy(3)) { return; }
	PullGymLever(3);
}
void ADirtbagDaySpot::OnChoose5()
{
	if (Game && Game->ChooseOnAScreen(4)) { return; }
	if (PickABivy(4)) { return; }
	PullGymLever(4);
}
void ADirtbagDaySpot::OnChoose6()
{
	if (Game && Game->ChooseOnAScreen(5)) { return; }
	if (PickABivy(5)) { return; }
	PullGymLever(5);
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
	// where you are going, so it asks the game where you are.
	//
	// **This used to infer it and the inference has stopped being true.**
	// With two walkable zones you could say "if I am going to town I must
	// be at the Lot, otherwise I must be in town" and be right every time.
	// The map went to eleven walkable zones on 2026-08-23, and that
	// sentence became false without becoming an error — the walk still
	// costs a number, and the number is somebody else's walk.
	const EDirtbagZone From = Game ? Game->CurrentZone : EDirtbagZone::Lot;
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

	// The sim's side of the trip, taken **once and up front**, before a
	// single frame of road is drawn.
	//
	// The screen that follows is pure presentation: it interpolates a clock
	// for display and moves a shape across a backdrop, and it applies
	// nothing. That ordering is deliberate -- if the player alt-F4s halfway
	// down the road, the world is already in the state of having arrived,
	// which is consistent. Applying at the end instead would leave a
	// half-taken trip on the floor.
	const bool bOnFoot = !UDirtbagSimLibrary::NeedsTheVan(DestinationZone);
	const double Hours = bOnFoot ? WalkHours() : DriveHours();

	TravelStartHour = Game->Day.Hour;

	// A walk costs time and nothing else: no fuel, no wear, no breakdown
	// roll. That is the point of connected ground -- the van is what buys
	// you rock, and everything else is your legs.
	const int32 Broke = bOnFoot ? -1 : Game->DriveVan(Hours);
	Game->PassHours(Hours);
	Game->SetVenue(ArriveAt);
	// Where in the world, alongside what rock -- the two answer different
	// questions and both change at the same moment, which is this one.
	// Set after `Hours`, because `WalkHours` above reads it as the origin
	// and would otherwise price the walk from where you are about to be.
	Game->CurrentZone = DestinationZone;

	BeginTravelScreen(bOnFoot, Hours, Broke);
}

void ADirtbagDaySpot::BeginTravelScreen(bool bOnFoot, double Hours,
                                        int32 Broke)
{
	FDirtbagTravelReadout& T = Game->TravelReadout;
	T.bActive = true;
	T.bOnFoot = bOnFoot;
	T.ToName = TravelName;
	T.Progress = 0.0;
	T.ShownHour = TravelStartHour;
	T.Minutes = Hours * 60.0;
	T.Fuel = bOnFoot ? 0.0 : Game->LastDriveFuel;
	// The one thing the road ever has to say beyond where you are going.
	// Said here rather than toasted on arrival, because a van giving up
	// halfway to the crag is a thing that happens *on the road* and the
	// road is finally on screen to show it.
	T.Note = Broke >= 0 ? Game->VanNews : FString();
	T.Backdrop = RoadBackdrop;
	T.VanImage = VanSprite;

	TravelElapsed = 0.f;
	TravelHoursTaken = Hours;

	// The screen needs frames. The spot does not tick otherwise -- it is a
	// trigger volume that answers keys -- so tick is switched on for the
	// length of the road and off again at the far kerb.
	SetActorTickEnabled(true);

	// Fade back in *behind* the road: the screen is what the player is
	// looking at now, so the black is no longer doing any work.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeSeconds,
			                                         FLinearColor::Black,
			                                         false, false);
		}
	}
}

bool ADirtbagDaySpot::SkipTravel()
{
	if (!Game || !Game->TravelReadout.bActive)
	{
		return false;
	}
	TravelElapsed = FMath::Max(TravelElapsed, TravelSeconds);
	return true;
}

void ADirtbagDaySpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Game || !Game->TravelReadout.bActive)
	{
		SetActorTickEnabled(false);
		return;
	}

	TravelElapsed += DeltaSeconds;
	const float Alpha =
	    FMath::Clamp(TravelElapsed / FMath::Max(0.05f, TravelSeconds), 0.f, 1.f);

	FDirtbagTravelReadout& T = Game->TravelReadout;
	T.Progress = Alpha;
	// Display only -- the real clock moved before the first frame of this.
	//
	// Wrapped rather than clamped, because a drive that leaves at 23:30 and
	// takes forty minutes arrives at 00:10, and a clamp would sit the shown
	// clock at 23:59 telling the player the trip took half an hour longer
	// than it did.
	T.ShownHour =
	    FMath::Fmod(TravelStartHour + TravelHoursTaken * Alpha, 24.0);

	if (Alpha < 1.f)
	{
		return;
	}

	// The far kerb. The teleport happens here rather than at the start so
	// that the player is not standing in the destination while the road is
	// still on screen -- the world behind the screen should match where the
	// screen says you are only once it says you have arrived.
	T.bActive = false;
	SetActorTickEnabled(false);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	// Re-checked here as well as at the start: the sim's side of the trip
	// is already applied by now, so a target that vanished mid-road would
	// leave the player standing at the near kerb having spent the hours.
	// Better to arrive nowhere than to spend the day twice.
	if (APawn* Pawn = (PC && TravelTarget) ? PC->GetPawn() : nullptr)
	{
		Pawn->TeleportTo(TravelTarget->GetActorLocation(),
		                 Pawn->GetActorRotation());
		if (PC)
		{
			// Face the way the destination faces, so arrivals are composed.
			PC->SetControlRotation(TravelTarget->GetActorRotation());
		}
	}

	if (!T.Note.IsEmpty())
	{
		Say(FString::Printf(TEXT("%s  %s"), *T.Note, *Game->VanLine()),
		    FColor::Red, 8.f);
	}
}

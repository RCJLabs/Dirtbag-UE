#include "DirtbagClimbWall.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#include "DirtbagGameInstance.h"
#include "DirtbagSimLibrary.h"

namespace
{
FString StyleText(EDirtbagStyle Style)
{
	switch (Style)
	{
	case EDirtbagStyle::Onsight:  return TEXT("Onsight");
	case EDirtbagStyle::Flash:    return TEXT("Flash");
	case EDirtbagStyle::Redpoint: return TEXT("Redpoint");
	case EDirtbagStyle::Sent:     return TEXT("Sent");
	case EDirtbagStyle::Fell:     return TEXT("Fell");
	}
	return TEXT("?");
}

// Key -1 appends a new line every time, so pressing a key four times stacks
// four identical messages. A stable key per kind of message replaces instead,
// which is what "you pressed the same key again" should look like.
enum : int32
{
	kToastPrompt = 4101,
	kToastClean = 4102,
	kToastResult = 4103,
};

void Toast(const FString& Msg, FColor Color = FColor::White,
           float Seconds = 4.0f, int32 Key = -1)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(Key, Seconds, Color, Msg);
	}
}
}  // namespace

ADirtbagClimbWall::ADirtbagClimbWall()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	HoldLine = CreateDefaultSubobject<USplineComponent>(TEXT("HoldLine"));
	HoldLine->SetupAttachment(Root);

	HoldMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HoldMarkers"));
	HoldMarkers->SetupAttachment(Root);
	HoldMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ApproachTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ApproachTrigger"));
	ApproachTrigger->SetupAttachment(Root);
	ApproachTrigger->SetBoxExtent(FVector(250.f, 250.f, 120.f));
	ApproachTrigger->SetRelativeLocation(FVector(0.f, 250.f, 120.f));

	SessionCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SessionCamera"));
	SessionCamera->SetupAttachment(Root);
	// A serviceable default frame: back off the wall, halfway up, looking in.
	SessionCamera->SetRelativeLocation(FVector(0.f, 450.f, 200.f));
	SessionCamera->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	Climber = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Climber"));
	Climber->SetupAttachment(Root);
	// Facing the rock, like the camera. See ClimberYaw.
	Climber->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Climber->SetVisibility(false);
	Climber->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
	    TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		HoldMarkerMesh = Sphere.Object;
	}
}

void ADirtbagClimbWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Applied here rather than only in the constructor: a wall already
	// placed in the level has its component transform serialized, so a new
	// constructor default would never reach it. This runs on every
	// construction, so existing walls face the right way the moment the
	// level reloads.
	Climber->SetRelativeRotation(FRotator(0.f, ClimberYaw, 0.f));

	HoldMarkers->ClearInstances();
	if (HoldMarkerMesh && HoldLine)
	{
		HoldMarkers->SetStaticMesh(HoldMarkerMesh);
		for (int32 i = 0; i < HoldLine->GetNumberOfSplinePoints(); i++)
		{
			const FVector Local =
			    HoldLine->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
			HoldMarkers->AddInstance(
			    FTransform(FRotator::ZeroRotator, Local, FVector(0.15f)));
		}
	}
}

void ADirtbagClimbWall::BeginPlay()
{
	Super::BeginPlay();

	Game = Cast<UDirtbagGameInstance>(GetGameInstance());
	if (Game)
	{
		// The board is the truth; the actor's route fields become display.
		// Asked by venue, not by "where is the player" — at BeginPlay nobody
		// has walked up to anything yet, and asking the live venue here is
		// what handed a crag wall a gym problem.
		Route = Game->GetRouteAt(Venue, BoardIndex);
		RouteName = Route.Name;
		Grade = Route.Grade;
		TrueGrade = Route.TrueGrade;
	}
	else
	{
		Route = UDirtbagSimLibrary::BuildRoute(
		    WorldSeed, RouteName, Grade, TrueGrade, RouteType,
		    EDirtbagDiscipline::Boulder);
	}
	RefreshBookName();
	SimRoute = DirtbagConvert::ToSim(Route);
	Session = UDirtbagSimLibrary::StartSession(ClimberStats);
	Memory = FDirtbagProjectMemory();

	ApproachTrigger->OnComponentBeginOverlap.AddDynamic(
	    this, &ADirtbagClimbWall::OnApproachBegin);
	ApproachTrigger->OnComponentEndOverlap.AddDynamic(
	    this, &ADirtbagClimbWall::OnApproachEnd);
}

void ADirtbagClimbWall::OnApproachBegin(UPrimitiveComponent*, AActor* OtherActor,
                                        UPrimitiveComponent*, int32, bool,
                                        const FHitResult&)
{
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}
	bPlayerNear = true;

	// Arriving at a wall is arriving somewhere. Doing this here rather than
	// asking anyone to set a flag means where the game thinks you are can
	// never disagree with what you are standing in front of.
	if (Game)
	{
		Game->SetVenue(Venue);
	}

	// The book may have changed since BeginPlay — a line named yesterday is
	// called something else today, and the page now carries the grade it
	// really went at rather than the guess.
	RefreshBookName();

	// Read the line from the ground before touching it — the sim's judgement
	// against the guidebook grade, so a sandbag still looks reasonable here.
	const FDirtbagClimber& Who = Game ? Game->Player.Climber : ClimberStats;
	const EDirtbagRouteRead Read = UDirtbagSimLibrary::ReadRoute(Who, Route);

	// Outdoors the guidebook has more to say than a name and a number: how
	// good the line is, and whether anyone has done it at all. A three-star
	// V4 and a no-star V4 are different decisions.
	FString Book;
	if (Game && IsOutdoors(Venue))
	{
		const FDirtbagCragLine Line = Game->GetCragLine(BoardIndex);
		if (Line.bIsProject)
		{
			// On a project the dirt is the obstacle, not the grade, so the
			// prompt leads with it.
			Book = FString::Printf(TEXT("   unclimbed — %s"),
			                       *Game->CleanlinessText(BoardIndex));
		}
		else if (Line.Stars > 0)
		{
			Book = TEXT("   ");
			for (int32 i = 0; i < Line.Stars; i++)
			{
				Book += TEXT("*");
			}
		}
	}

	Toast(FString::Printf(
	          TEXT("%s  %s%s — %s   (E to climb)"), *RouteName,
	          *UDirtbagSimLibrary::GradeName(Grade, EDirtbagDiscipline::Boulder),
	          *Book, *UDirtbagSimLibrary::ReadRouteText(Read)),
	      FColor::Cyan, 5.f, kToastPrompt);

	// Who is holding the rope, on a line that needs one. This is the first
	// thing in the game rapport buys that nothing else can, so it is said at
	// the moment it matters rather than left to be discovered by pressing E.
	if (Game && UDirtbagSimLibrary::NeedsABelayer(Route))
	{
		const int32 Left = Game->BurnsHeldToday() - Game->RopedBurnsToday;
		Toast(Game->CanTieIn()
		          ? FString::Printf(TEXT("%s  (%d %s left)"), *Game->BelayLine(),
		                            Left, Left == 1 ? TEXT("burn") : TEXT("burns"))
		          : Game->RopeRefusal(),
		      Game->CanTieIn() ? FColor::Cyan : FColor::Orange, 5.f,
		      kToastResult);
	}

	// And where the body is, which is the half a player cannot see. Only
	// once a session is under way — before that everyone is cold and
	// saying so is noise.
	if (Game && Game->Day.bAtGym &&
	    Game->ReadSession() != EDirtbagSessionAdvice::Ready)
	{
		Toast(Game->SessionAdviceText(),
		      Game->ReadSession() == EDirtbagSessionAdvice::Wrecked
		          ? FColor::Orange
		          : FColor::Silver,
		      5.f, kToastResult);
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		if (InputComponent && !bBoundInput)
		{
			InputComponent->BindKey(EKeys::E, IE_Pressed, this,
			                        &ADirtbagClimbWall::OnInteract);
			InputComponent->BindKey(EKeys::C, IE_Pressed, this,
			                        &ADirtbagClimbWall::OnClean);
			InputComponent->BindKey(EKeys::B, IE_Pressed, this,
			                        &ADirtbagClimbWall::OnAskBeta);
			InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this,
			                        &ADirtbagClimbWall::OnHoldPressed);
			InputComponent->BindKey(EKeys::SpaceBar, IE_Released, this,
			                        &ADirtbagClimbWall::OnHoldReleased);
			bBoundInput = true;
		}
	}
}

void ADirtbagClimbWall::OnApproachEnd(UPrimitiveComponent*, AActor* OtherActor,
                                      UPrimitiveComponent*, int32)
{
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}
	bPlayerNear = false;
	if (Phase == EPhase::Idle)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			DisableInput(PC);
		}
	}
}

void ADirtbagClimbWall::OnInteract()
{
	if (Phase == EPhase::Idle && bPlayerNear)
	{
		StartAttempt();
	}
}

void ADirtbagClimbWall::OnClean()
{
	// Cleaning is a day action, not a session one: it costs hours and
	// energy whether or not you then pull on. Indoors it is nonsense, and
	// saying so is better than a key that silently does nothing.
	if (!Game || Phase != EPhase::Idle)
	{
		return;
	}
	if (!IsOutdoors(Venue))
	{
		Toast(TEXT("Someone else cleans the holds here."), FColor::Silver,
		      4.f, kToastClean);
		return;
	}
	if (Game->CleanLine(BoardIndex, CleanHoursPerPress) <= 0.0)
	{
		Toast(TEXT("It is as clean as it is going to get."), FColor::Silver,
		      4.f, kToastClean);
		return;
	}
	Toast(FString::Printf(TEXT("%.0f minutes on the brush.  %s"),
	                      CleanHoursPerPress * 60.f,
	                      *Game->CleanlinessText(BoardIndex)),
	      FColor::Silver, 4.f, kToastClean);
}

void ADirtbagClimbWall::OnAskBeta()
{
	if (!Game || Phase != EPhase::Idle)
	{
		return;
	}
	if (!IsOutdoors(Venue))
	{
		Toast(TEXT("The setter's beta is on the tag."), FColor::Silver, 4.f,
		      kToastClean);
		return;
	}
	FString Who;
	const double Learned = Game->AskForBeta(BoardIndex, Who);
	if (Learned <= 0.0)
	{
		// Which is the honest answer on an unclimbed line: nobody has beta
		// on something nobody has done.
		Toast(TEXT("Nobody here has done it."), FColor::Silver, 4.f,
		      kToastClean);
		return;
	}
	Toast(FString::Printf(TEXT("%s walks you through it."), *Who),
	      FColor::Cyan, 5.f, kToastClean);
}

void ADirtbagClimbWall::StartAttempt()
{
	if (HoldLine->GetNumberOfSplinePoints() < 2)
	{
		Toast(TEXT("HoldLine needs spline points before anyone can climb."),
		      FColor::Red);
		return;
	}

	// No partner, no pitch. A boulder needs nobody; a bolted line needs
	// somebody at the bottom of it, and how long they will stand there is
	// rapport. Refused here rather than at the toast, because a rule you can
	// walk past by pressing E again is not a rule.
	if (Game && UDirtbagSimLibrary::NeedsABelayer(Route) && !Game->CanTieIn())
	{
		Toast(Game->RopeRefusal(), FColor::Orange, 5.f);
		return;
	}
	if (Game && UDirtbagSimLibrary::NeedsABelayer(Route))
	{
		Game->RopedBurnsToday++;
	}

	bLiveSession = bInteractive;
	if (Game)
	{
		// Day-integrated: session, ledger, time, energy, training all move
		// together through the game instance.
		if (bLiveSession)
		{
			Live = Game->BeginLiveFor(Route);
		}
		else
		{
			Current = Game->ReplayAttempt(Route);
		}
	}
	else if (bLiveSession)
	{
		// Legacy standalone (no game instance): the player drives; the sim
		// still arbitrates every move.
		const dirtbag::Rng SessionRng = dirtbag::Rng::FromStream(
		    TCHAR_TO_UTF8(*SessionSeed), dirtbag::Stream::Session);
		const dirtbag::SessionState SimSession = DirtbagConvert::ToSim(Session);
		const dirtbag::ProjectMemory SimMemory = DirtbagConvert::ToSim(Memory);
		Live = dirtbag::BeginAttempt(
		    dirtbag::DeriveAttemptRng(SessionRng, SimMemory, SimRoute),
		    dirtbag::BuildSessionAttemptInput(SimSession, SimMemory,
		                                      DirtbagConvert::ToSim(ClimberStats),
		                                      SimRoute, dirtbag::Conditions{}));
	}
	else
	{
		// Legacy standalone bot replay.
		Current = UDirtbagSimLibrary::AttemptInSession(
		    SessionSeed, Session, Memory, ClimberStats, Route);
	}

	TimelineIndex = 0;
	HoldIndex = 0;
	bReachLeft = false;
	bCharging = false;
	Charge = 0.f;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->SetViewTargetWithBlend(this, 0.5f);
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->SetActorHiddenInGame(true);
		}
	}

	Climber->SetWorldLocation(HoldLocation(0));
	// Location is set every move and rotation never was, which is the whole
	// bug. Set it once here too, so a wall spawned at runtime — which never
	// sees OnConstruction — cannot climb backwards either.
	Climber->SetRelativeRotation(FRotator(0.f, ClimberYaw, 0.f));
	Climber->SetVisibility(true);

	const int32 AttemptNo =
	    Game ? Game->AttemptsOn(Route) + (bLiveSession ? 1 : 0)
	         : Memory.Attempts + (bLiveSession ? 1 : 0);
	Toast(FString::Printf(TEXT("Attempt %d"), AttemptNo), FColor::Yellow);

	if (MountAnim)
	{
		Phase = EPhase::Mounting;
		PlayAnim(MountAnim, false);
		GetWorldTimerManager().SetTimer(PhaseTimer, this,
		                                &ADirtbagClimbWall::BeginSessionBody,
		                                MountAnim->GetPlayLength(), false);
	}
	else
	{
		BeginSessionBody();
	}
}

void ADirtbagClimbWall::BeginSessionBody()
{
	PlayAnim(HangIdleAnim, true);
	if (bLiveSession)
	{
		Phase = EPhase::AtStance;
		Toast(TEXT("HOLD Space to load the move. Release in the window to "
		           "latch it; release early to shake out."),
		      FColor::Cyan, 6.f);
	}
	else
	{
		ScheduleNextMove();
	}
}

void ADirtbagClimbWall::ScheduleNextMove()
{
	if (TimelineIndex >= Current.Timeline.Num())
	{
		FinishAttempt();
		return;
	}
	// The hesitation IS the odds read: a sure move flows, a thin one
	// visibly gathers itself first.
	const double Odds = Current.Timeline[TimelineIndex].Odds;
	const float Wait =
	    BaseHesitation + HesitationPerRisk * static_cast<float>(1.0 - Odds);
	Phase = EPhase::AtStance;
	GetWorldTimerManager().SetTimer(PhaseTimer, this,
	                                &ADirtbagClimbWall::BeginMove, Wait, false);
}

void ADirtbagClimbWall::BeginMove()
{
	StageMoveResult(Current.Timeline[TimelineIndex].bSuccess);
}

void ADirtbagClimbWall::OnHoldPressed()
{
	if (bLiveSession && Phase == EPhase::AtStance)
	{
		bCharging = true;
		Charge = 0.f;
	}
}

void ADirtbagClimbWall::OnHoldReleased()
{
	if (!bCharging)
	{
		return;
	}
	bCharging = false;

	if (Charge < SweetWindowStart)
	{
		// The release verb: settle back into the stance and shake.
		const double Recovered = dirtbag::ShakeOut(Live);
		if (Recovered > 0.5)
		{
			Toast(FString::Printf(TEXT("shake  -%.0f pump"), Recovered),
			      FColor::Cyan, 1.5f);
		}
		else if (Recovered < -0.5)
		{
			Toast(TEXT("nothing to milk here — that cost you"),
			      FColor::Orange, 1.5f);
		}
		return;
	}

	// Latched: execution peaks dead-center in the window, tapers to the edges.
	const float Mid = (SweetWindowStart + SweetWindowEnd) * 0.5f;
	const float Half = FMath::Max(0.01f, (SweetWindowEnd - SweetWindowStart) * 0.5f);
	const float Off = FMath::Clamp(FMath::Abs(Charge - Mid) / Half, 0.f, 1.f);
	CommitMove(FMath::Lerp(PerfectExecution, EdgeExecution, Off));
}

void ADirtbagClimbWall::CommitMove(double Execution)
{
	const dirtbag::MoveResult MR = dirtbag::StepMove(Live, Execution);
	StageMoveResult(MR.success);
}

void ADirtbagClimbWall::StageMoveResult(bool bSuccess)
{
	MoveFrom = Climber->GetComponentLocation();
	MoveAlpha = 0.f;

	if (bSuccess)
	{
		const int32 NextHold =
		    FMath::Min(HoldIndex + 1, HoldLine->GetNumberOfSplinePoints() - 1);
		MoveTo = HoldLocation(NextHold);
		Phase = EPhase::Moving;
		PlayAnim(bReachLeft ? ReachLeftAnim : ReachRightAnim, false);
		bReachLeft = !bReachLeft;
	}
	else
	{
		// The peel: straight down to the mat (this actor sits at mat height).
		MoveTo = FVector(MoveFrom.X, MoveFrom.Y, GetActorLocation().Z + 30.f);
		Phase = EPhase::Falling;
		PlayAnim(FallAnim, false);
	}
}

void ADirtbagClimbWall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The grip meter: charging past full is the death grip — the move fires
	// itself, badly.
	if (bCharging)
	{
		Charge += DeltaSeconds / FMath::Max(0.05f, ChargeTime);
		if (Charge >= 1.f)
		{
			bCharging = false;
			Toast(TEXT("over-gripped"), FColor::Orange, 1.5f);
			CommitMove(OvergripExecution);
		}
	}

	if (Phase != EPhase::Idle)
	{
		UpdateHud();
	}

	if (Phase != EPhase::Moving && Phase != EPhase::Falling)
	{
		return;
	}

	const float Duration = Phase == EPhase::Moving ? MoveDuration : FallDuration;
	MoveAlpha = FMath::Min(1.f, MoveAlpha + DeltaSeconds / FMath::Max(0.05f, Duration));
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, MoveAlpha, 2.f);
	Climber->SetWorldLocation(FMath::Lerp(MoveFrom, MoveTo, Eased));

	if (MoveAlpha < 1.f)
	{
		return;
	}

	if (Phase == EPhase::Moving)
	{
		HoldIndex = FMath::Min(HoldIndex + 1, HoldLine->GetNumberOfSplinePoints() - 1);
		if (bLiveSession)
		{
			if (dirtbag::AttemptOver(Live))
			{
				FinishLiveAttempt();  // topped out
			}
			else
			{
				PlayAnim(HangIdleAnim, true);
				Phase = EPhase::AtStance;
			}
		}
		else
		{
			TimelineIndex++;
			PlayAnim(HangIdleAnim, true);
			ScheduleNextMove();
		}
	}
	else  // landed
	{
		if (bLiveSession)
		{
			FinishLiveAttempt();
		}
		else
		{
			FinishAttempt();
		}
	}
}

void ADirtbagClimbWall::FinishLiveAttempt()
{
	// The attempt is over; the ledger gets paid through the sim's own
	// accounting — never presentation-side bookkeeping.
	if (Game)
	{
		Current = Game->CommitLiveFor(Live);
	}
	else
	{
		const dirtbag::AttemptResult SimResult = dirtbag::FinishAttempt(Live);
		Current = DirtbagConvert::FromSim(SimResult);
		dirtbag::SessionState SimSession = DirtbagConvert::ToSim(Session);
		dirtbag::ProjectMemory SimMemory = DirtbagConvert::ToSim(Memory);
		dirtbag::CommitAttempt(SimSession, SimMemory, SimRoute, SimResult);
		Session = DirtbagConvert::FromSim(SimSession);
		Memory = DirtbagConvert::FromSim(SimMemory);
	}
	FinishAttempt();
}

void ADirtbagClimbWall::FinishAttempt()
{
	Phase = EPhase::Ending;
	const FString GradeText =
	    UDirtbagSimLibrary::GradeName(Grade, EDirtbagDiscipline::Boulder);
	if (Current.bSent)
	{
		if (TopOutAnim)
		{
			PlayAnim(TopOutAnim, false);
		}
		Toast(FString::Printf(TEXT("%s  %s  —  %s"), *RouteName, *GradeText,
		                      *StyleText(Current.Style)),
		      FColor::Green, 5.f);

		// If nobody had done it, the naming is now yours. The wall only
		// raises the flag; what the prompt looks like is a widget's job.
		if (Game && Game->CanNameLine(BoardIndex))
		{
			Game->OfferNaming(BoardIndex);
		}
	}
	else
	{
		const double SkinLeft =
		    Game ? Game->Day.Session.SkinLeft : Session.SkinLeft;
		Toast(FString::Printf(TEXT("Off at move %d of %d.  Skin left: %.1f"),
		                      Current.Highpoint + 1, Route.Moves.Num(),
		                      SkinLeft),
		      FColor::Orange, 5.f);
	}
	GetWorldTimerManager().SetTimer(PhaseTimer, this,
	                                &ADirtbagClimbWall::EndSession, EndPause,
	                                false);
}

void ADirtbagClimbWall::EndSession()
{
	Climber->SetVisibility(false);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->SetActorHiddenInGame(false);
			PC->SetViewTargetWithBlend(Pawn, 0.5f);
		}
	}
	Phase = EPhase::Idle;

	// The session panel is drawn off this flag, so leaving it set would
	// pin a dead route's pump bar to the screen for the rest of the day.
	if (Game)
	{
		Game->SessionReadout = FDirtbagSessionReadout();
	}

	if (bPlayerNear)
	{
		Toast(TEXT("Press E to go again."), FColor::Cyan);
	}
}

void ADirtbagClimbWall::UpdateHud()
{
	if (!Game)
	{
		return;
	}

	// Publish, never draw: ADirtbagHUD owns how any of this looks.
	FDirtbagSessionReadout& S = Game->SessionReadout;
	S.bActive = true;
	S.RouteLine = FString::Printf(
	    TEXT("%s      %s"), *RouteName,
	    *UDirtbagSimLibrary::GradeName(Grade, EDirtbagDiscipline::Boulder));
	S.WindowStart = SweetWindowStart;
	S.WindowEnd = SweetWindowEnd;
	S.Grip = bCharging ? Charge : -1.0;

	if (bLiveSession)
	{
		S.Pump = Live.pump;
		S.Odds = Phase == EPhase::AtStance
		             ? dirtbag::PeekOdds(Live, PerfectExecution)
		             : -1.0;
	}
	else if (Current.Timeline.IsValidIndex(TimelineIndex))
	{
		// The replay reads off its own script, so a watched attempt shows
		// the same rising pump and thinning odds a driven one does.
		const FDirtbagMoveResult& Move = Current.Timeline[TimelineIndex];
		S.Pump = Move.PumpAfter;
		S.Odds = Phase == EPhase::AtStance ? Move.Odds : -1.0;
	}
}

void ADirtbagClimbWall::RefreshBookName()
{
	// Indoors there is no book — a gym board is a ladder that resets, and
	// asking the crag for a line here is what once handed a gym wall a
	// boulder problem off the wrong rock.
	if (!Game || !IsOutdoors(Venue))
	{
		return;
	}
	const FDirtbagCragLine Line = Game->GetCragLineAt(Venue, BoardIndex);
	if (Line.Route.Name.IsEmpty())
	{
		return;
	}
	if (!Line.DisplayName.IsEmpty())
	{
		RouteName = Line.DisplayName;
	}
	// A first ascent replaces the book's guess with what it really went at,
	// so the number on screen moves with the name.
	Grade = Line.Route.Grade;
}

void ADirtbagClimbWall::PlayAnim(UAnimSequence* Anim, bool bLoop)
{
	if (Anim)
	{
		Climber->PlayAnimation(Anim, bLoop);
	}
}

FVector ADirtbagClimbWall::HoldLocation(int32 Index) const
{
	return HoldLine->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

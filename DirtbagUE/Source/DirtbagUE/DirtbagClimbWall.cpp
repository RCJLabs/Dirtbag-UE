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
	// 4101 (kToastPrompt) and 4103 (kToastResult) are gone: everything
	// they carried -- the route line, the belayer, the body advice -- was
	// standing state rather than news, and is a prompt line now. Their
	// numbers are left unclaimed rather than reused, because two kinds of
	// message on one slot is what caused the bug that started this.
	kToastClean = 4102,
	// Its own key, or the brush toast would replace it the next press and
	// the one message worth reading would be the one you never see.
	kToastClaim = 4104,
	// Level-setup mistakes. Long-lived and on their own slot, because they
	// are aimed at whoever placed the actor rather than at a player, and a
	// four-second red flash during a playtest is the one surface that
	// guarantees he misses it.
	kToastSetup = 4105,
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
	// Facing is derived in FaceTheRock() rather than set here: at
	// construction the spline may not exist yet, and a fixed angle was
	// wrong twice. This is only the harmless starting value.
	Climber->SetRelativeRotation(FRotator(0.f, ClimberYaw, 0.f));
	Climber->SetVisibility(false);
	Climber->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
	    TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		HoldMarkerMesh = Sphere.Object;
	}
}

void ADirtbagClimbWall::FaceTheRock()
{
	if (!Climber)
	{
		return;
	}
	if (!bFaceAwayFromCamera || !SessionCamera)
	{
		Climber->SetRelativeRotation(FRotator(0.f, ClimberYaw, 0.f));
		return;
	}

	// Away from the camera, flattened: a climber leans and reaches, but
	// they do not tip over, so only yaw is ever derived.
	FVector Away = Climber->GetComponentLocation() -
	               SessionCamera->GetComponentLocation();
	Away.Z = 0.f;
	if (Away.IsNearlyZero())
	{
		// Degenerate: the climber is directly under the camera, which
		// happens for one frame before the spline is read. Keep whatever
		// they had rather than snapping to an arbitrary axis.
		return;
	}

	// The mesh's own forward is subtracted, because "yaw 0" does not mean
	// "faces +X" for every skeletal mesh -- and assuming it did is what
	// made the first two attempts wrong.
	const float Facing = Away.Rotation().Yaw - MeshForwardYaw;
	Climber->SetWorldRotation(FRotator(0.f, Facing, 0.f));
}

void ADirtbagClimbWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Applied here rather than only in the constructor: a wall already
	// placed in the level has its component transform serialized, so a new
	// constructor default would never reach it. This runs on every
	// construction, so an existing wall faces the right way the moment you
	// move the spline, the camera, or the actor -- which is the whole
	// point of deriving it. Park the climber on the first hold first, so
	// the derivation has a real position to aim from.
	if (HoldLine && HoldLine->GetNumberOfSplinePoints() > 0)
	{
		Climber->SetWorldLocation(HoldLocation(0));
	}
	FaceTheRock();

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

	// The shot as placed, taken once and never taken again. Everything the
	// camera does during a session moves it, so reading it a second time
	// would read this actor's own output back as the author's intent.
	if (SessionCamera && !bShotCaptured)
	{
		RestOffset = SessionCamera->GetRelativeLocation();
		RestRotation = SessionCamera->GetRelativeRotation();
		RestFov = SessionCamera->FieldOfView;
		ShotLocal = RestOffset;
		bShotCaptured = true;
	}

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

void ADirtbagClimbWall::PushPrompt()
{
	if (!Game)
	{
		return;
	}
	if (!bPlayerNear || Phase != EPhase::Idle)
	{
		// Nothing to prompt while anything is running on this wall: the
		// session panel is the interaction then, and two things claiming
		// to say what the keys do is worse than one. Phase rather than
		// bLiveSession, because a *watched* attempt is just as much a
		// session and "E to climb" is just as wrong underneath it.
		Game->ClearPrompt(this);
		return;
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

	// What is here, who will hold the rope, and where the body is. All
	// three are standing state — true for as long as you are stood at the
	// bottom of this line — so all three are prompt lines rather than
	// toasts that expire while you are still deciding.
	//
	// The sorting found a live bug doing this: the belayer line and the
	// body advice were pushed through the *same* keyed toast slot two
	// statements apart, so on a roped line at the gym when you were tired,
	// the advice silently replaced the belayer and you were never told who
	// was holding the rope. Separate lines cannot do that to each other.
	TArray<FDirtbagPromptLine> Lines;

	FDirtbagPromptLine What;
	What.Text = FString::Printf(
	    TEXT("%s  %s%s — %s   (E to climb)"), *RouteName,
	    *UDirtbagSimLibrary::GradeName(Grade, EDirtbagDiscipline::Boulder),
	    *Book, *UDirtbagSimLibrary::ReadRouteText(Read));
	// An unclimbed line is the one thing on this screen worth walking
	// across a valley for, and Book leads with it.
	What.Tone = Book.Contains(TEXT("unclimbed")) ? EDirtbagPromptTone::Good
	                                             : EDirtbagPromptTone::Plain;
	Lines.Add(What);

	// Who is holding the rope, on a line that needs one. This is the first
	// thing in the game rapport buys that nothing else can, so it is said at
	// the moment it matters rather than left to be discovered by pressing E.
	if (Game && UDirtbagSimLibrary::NeedsABelayer(Route))
	{
		const int32 Left = Game->BurnsHeldToday() - Game->RopedBurnsToday;
		FDirtbagPromptLine Rope;
		Rope.Text =
		    Game->CanTieIn()
		        ? FString::Printf(TEXT("%s  (%d %s left)"), *Game->BelayLine(),
		                          Left, Left == 1 ? TEXT("burn") : TEXT("burns"))
		        : Game->RopeRefusal();
		Rope.Tone = Game->CanTieIn() ? EDirtbagPromptTone::Plain
		                             : EDirtbagPromptTone::Blocked;
		Lines.Add(Rope);
	}

	// And where the body is, which is the half a player cannot see. Only
	// once a session is under way — before that everyone is cold and
	// saying so is noise.
	if (Game && Game->Day.bAtGym &&
	    Game->ReadSession() != EDirtbagSessionAdvice::Ready)
	{
		FDirtbagPromptLine Body;
		Body.Text = Game->SessionAdviceText();
		Body.Tone = Game->ReadSession() == EDirtbagSessionAdvice::Wrecked
		                ? EDirtbagPromptTone::Blocked
		                : EDirtbagPromptTone::Plain;
		Lines.Add(Body);
	}

	if (Game)
	{
		Game->SetPrompt(this, Lines);
	}
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

	// What is here, who holds the rope, where the body is, and what E
	// does about it. Rebuilt rather than said once, because every one of
	// those changes while you stand here: the brush changes the
	// cleanliness, a burn spends a belayer, a session leaves you tired.
	PushPrompt();

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
	if (Game)
	{
		Game->ClearPrompt(this);
	}
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
	const double Gained = Game->CleanLine(BoardIndex, CleanHoursPerPress);
	if (Gained <= 0.0)
	{
		Toast(TEXT("It is as clean as it is going to get."), FColor::Silver,
		      4.f, kToastClean);
		return;
	}
	// The prompt leads with cleanliness on a project, and the brush just
	// changed it. Said after the toast so the news reads as news and the
	// standing state updates underneath it.
	Toast(FString::Printf(TEXT("%.0f minutes on the brush.  %s"),
	                      CleanHoursPerPress * 60.f,
	                      *Game->CleanlinessText(BoardIndex)),
	      FColor::Silver, 4.f, kToastClean);

	// The one time brushing is news rather than housekeeping: this is the
	// press that puts your name on the line. Said once, when it happens.
	const FString Claim = Game->ClaimText(BoardIndex, Gained);
	if (!Claim.IsEmpty())
	{
		Toast(Claim, FColor::White, 6.f, kToastClaim);
	}
	PushPrompt();
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
		// Not player text. This is a level that was placed wrong, and the
		// person who needs to read it is the one holding the editor -- so
		// it goes to the log, where it survives the playtest, and to a
		// toast that names the actor and does not expire in four seconds
		// while he is looking somewhere else.
		UE_LOG(LogDirtbagSetup, Warning,
		       TEXT("%s: HoldLine has %d spline points; a climbable line "
		            "needs at least 2."),
		       *GetName(), HoldLine->GetNumberOfSplinePoints());
		Toast(FString::Printf(
		          TEXT("SETUP: %s has no HoldLine spline points."), *GetName()),
		      FColor::Red, 30.f, kToastSetup);
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
	// Rotation after location, always: the derivation reads where the
	// climber actually is, so setting it first would aim from the last
	// hold. Location is set every move and rotation never was, which was
	// the original bug; deriving it is the fix for the two that followed.
	FaceTheRock();
	Climber->SetVisibility(true);

	const int32 AttemptNo =
	    Game ? Game->AttemptsOn(Route) + (bLiveSession ? 1 : 0)
	         : Memory.Attempts + (bLiveSession ? 1 : 0);
	if (Game)
	{
		// On the session panel rather than in a four-second flash. "Attempt
		// 14 on the same problem" is true for the whole attempt and is most
		// of what a session feels like; it should not be gone by the time
		// you are on the crux.
		Game->SessionReadout.Attempt = AttemptNo;
	}

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

	// Both branches have moved off Idle by now, so this takes the prompt
	// down. It goes back up in EndSession, rebuilt from whatever the
	// attempt changed.
	PushPrompt();
}

void ADirtbagClimbWall::BeginSessionBody()
{
	PlayAnim(HangIdleAnim, true);
	if (bLiveSession)
	{
		Phase = EPhase::AtStance;
		// The verb used to be a six-second toast at the top of the route,
		// which is the one moment a first-time player is looking at the
		// climber rather than at the text. It lives on the session panel
		// now, under the grip bar it describes, until the first latch
		// proves it is not needed.
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

	// You have done it once, so the reminder stops -- for good, not for
	// this attempt. Set on the latch rather than on the press, because
	// holding Space and letting go too early is exactly the mistake the
	// line is there to explain.
	if (Game)
	{
		Game->bLearnedTheVerb = true;
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
		// After UpdateHud, deliberately: the shot reads the readout, and
		// reading it before it was written for this frame would frame
		// every move on the previous move's difficulty.
		UpdateCamera(DeltaSeconds);
	}

	if (Phase != EPhase::Moving && Phase != EPhase::Falling)
	{
		return;
	}

	const float Duration = Phase == EPhase::Moving ? MoveDuration : FallDuration;
	MoveAlpha = FMath::Min(1.f, MoveAlpha + DeltaSeconds / FMath::Max(0.05f, Duration));
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, MoveAlpha, 2.f);
	Climber->SetWorldLocation(FMath::Lerp(MoveFrom, MoveTo, Eased));
	// Re-derived as they travel: on a long traverse the line from the
	// camera swings round, and a facing computed once at the first hold
	// would be stale by the last one.
	FaceTheRock();

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
		// The sentence leads and the numbers follow it. Phase 0's gate is
		// that a watcher can tell how close that was *without reading a
		// number*, and "off at move 9 of 12" is a number doing the work
		// the staging is supposed to do. It stays -- a player who wants
		// the count should have it -- but it stops being the first thing
		// said, and what it says is now the sim's judgement rather than
		// arithmetic the reader has to do.
		const double Close = UDirtbagSimLibrary::HowClose(
		    Current, Route.Moves.Num());
		Toast(FString::Printf(TEXT("%s   (move %d of %d, skin %.1f)"),
		                      *UDirtbagSimLibrary::HowCloseText(Close),
		                      Current.Highpoint + 1, Route.Moves.Num(),
		                      SkinLeft),
		      // A near miss reads warm and a nothing go reads grey, so the
		      // colour carries it too for anybody not reading at all.
		      Close >= 0.6 ? FColor::Yellow : FColor::Orange, 6.f);
	}
	GetWorldTimerManager().SetTimer(PhaseTimer, this,
	                                &ADirtbagClimbWall::EndSession, EndPause,
	                                false);
}

void ADirtbagClimbWall::EndSession()
{
	Climber->SetVisibility(false);
	// Back to the shot as placed, so nothing an attempt did to the camera
	// survives it and the next one starts from the author's framing rather
	// than from wherever the last crux left it.
	RestCamera();
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

	// "Press E to go again" was a toast saying one third of what the
	// approach prompt says. The session is over, so the prompt comes back
	// -- with the grade, the read, the belayer and how many burns are
	// left, all of which changed while you were on the wall.
	PushPrompt();
}

void ADirtbagClimbWall::RestCamera()
{
	if (!SessionCamera || !bShotCaptured)
	{
		return;
	}
	SessionCamera->SetRelativeLocation(RestOffset);
	SessionCamera->SetRelativeRotation(RestRotation);
	SessionCamera->SetFieldOfView(RestFov);
	ShotLocal = RestOffset;
	Tension = 0.f;
	SwayTime = 0.f;
}

void ADirtbagClimbWall::UpdateCamera(float DeltaSeconds)
{
	if (!bCameraFollows || !SessionCamera || !Climber || !bShotCaptured ||
	    Phase == EPhase::Idle)
	{
		// Idle leaves the placed shot exactly alone, so walking up to a
		// wall looks like whatever Evan framed in the editor.
		return;
	}

	// How hard the move in front of us is. Straight off the readout the HUD
	// reads, so the shot and the bars can never disagree -- and so a
	// watched attempt is framed like a driven one for free, because the
	// readout already answers for both. Odds below zero means no move is
	// pending (mid-move, or falling), and the framing holds where it was
	// rather than snapping open halfway through a lunge.
	const double Odds = Game ? Game->SessionReadout.Odds : -1.0;
	if (Odds >= 0.0)
	{
		Tension = FMath::FInterpTo(Tension, static_cast<float>(1.0 - Odds),
		                           DeltaSeconds, TensionEase);
	}

	// Out from the wall, and how far. Both come from the authored offset --
	// the direction because the container cannot see which way this wall
	// faces, and the distance because the shot Evan placed is the wide one.
	FVector Out = RestOffset;
	Out.Z = 0.f;
	if (Out.IsNearlyZero())
	{
		// A camera placed directly above or below the root has no "out",
		// and there is nothing here that could invent one.
		return;
	}
	const float Wide = Out.Size();
	Out.Normalize();

	// Level with the climber and `Distance` out from the wall. The height
	// lead is done by *aiming* rather than by rising: raise the camera as
	// well and the two cancel, which is the version of this I wrote first.
	const FVector Focus = Climber->GetComponentLocation();
	const FVector Wanted =
	    GetActorTransform().InverseTransformPosition(Focus) +
	    Out * (Wide * (1.f - CameraTightenBy * Tension));

	// Smoothed on our own value rather than on the camera's, because the
	// camera's includes last frame's sway -- feeding that back in turns a
	// sway into a drift, which is the other version of this I wrote first.
	ShotLocal = FMath::VInterpTo(ShotLocal, Wanted, DeltaSeconds, CameraEase);

	// Pump, said without a bar. Squared so it is invisible for the first
	// half of a route and unmistakable at the top -- which is also how
	// being pumped works. Two frequencies that do not divide into each
	// other, because a clean sine reads as a machine rather than as
	// somebody holding on.
	const double Pump = Game ? Game->SessionReadout.Pump : 0.0;
	const float Pumped =
	    FMath::Clamp(static_cast<float>(Pump) / 100.f, 0.f, 1.f);
	SwayTime += DeltaSeconds * PumpSwaySpeed;
	const float Amount = PumpSway * Pumped * Pumped;
	// Lateral to the *shot*, not to the actor. Swaying along the actor's Y
	// would push the camera into and out of the wall on any wall whose
	// camera is not placed along Y -- the same assumption about somebody
	// else's level that cost two builds on the climber's facing.
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Out);
	const FVector Sway = Right * (FMath::Sin(SwayTime) * Amount) +
	                     FVector::UpVector *
	                         (FMath::Sin(SwayTime * 1.37f) * Amount * 0.6f);

	SessionCamera->SetRelativeLocation(ShotLocal + Sway);

	// Aim a little above their hands, so the frame carries the rock they
	// are going to rather than the rock they have done.
	const FVector Look =
	    (Focus + FVector(0.f, 0.f, CameraLead)) -
	    SessionCamera->GetComponentLocation();
	if (!Look.IsNearlyZero())
	{
		SessionCamera->SetWorldRotation(Look.Rotation());
	}

	SessionCamera->SetFieldOfView(RestFov - CameraNarrowBy * Tension);
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
	S.bShowTheVerb = !Game->bLearnedTheVerb;

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

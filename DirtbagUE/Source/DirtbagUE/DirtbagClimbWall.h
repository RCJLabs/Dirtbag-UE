// The Phase 0 staged-replay wall: walk up, press E, watch the sim's attempt
// acted out on the hold spline. Presentation only — every outcome comes from
// the sim (AttemptInSession); this actor just stages the returned timeline:
// odds → hesitation, success → reach to the next hold, failure → the fall.
//
// Editor setup (notes/phase0-blockout.md): place in front of a wall mesh,
// drag the HoldLine spline points up the wall, assign a skeletal mesh and
// the four animations in the Details panel. Everything else is handled.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DirtbagSimTypes.h"

#include "DirtbagClimbWall.generated.h"

class UAnimSequence;
class UBoxComponent;
class UCameraComponent;
class UDirtbagGameInstance;
class UInstancedStaticMeshComponent;
class USkeletalMeshComponent;
class USplineComponent;

UCLASS()
class ADirtbagClimbWall : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagClimbWall();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// --- Components -----------------------------------------------------

	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<USceneComponent> Root;

	/** The route line: one point per hold, bottom to top. Edit in-level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<USplineComponent> HoldLine;

	/** Small spheres rendered at each spline point so holds read at a glance. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UInstancedStaticMeshComponent> HoldMarkers;

	/** Walk into this to get the prompt; E starts the session. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UBoxComponent> ApproachTrigger;

	/** The watched-session frame. Position it to see the whole wall. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UCameraComponent> SessionCamera;

	/** The climber puppet. Assign the template's Quinn/Manny mesh. Not a
	 *  Character — it is placed, never simulated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<USkeletalMeshComponent> Climber;

	// --- Assets (assign in Details) -------------------------------------

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> HangIdleAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> ReachLeftAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> ReachRightAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> FallAnim;

	/** Optional: played once when the session starts (grab wall from ground). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> MountAnim;

	/** Optional: played on a send (climb up over the top). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> TopOutAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag")
	TObjectPtr<UStaticMesh> HoldMarkerMesh;

	/** True: the player drives each move with HOLD TO CLIMB (Space).
	 *  False: the bot-staged replay, as before. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag")
	bool bInteractive = true;

	// --- Route / climber config -----------------------------------------

	/** With a DirtbagGameInstance present, this wall carries gym-board
	 *  problem #BoardIndex and the fields below become fallbacks (used only
	 *  in levels without the game instance, e.g. isolated test maps). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	int32 BoardIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	FString WorldSeed = TEXT("gym-1");

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	FString SessionSeed = TEXT("gym-1");

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	FString RouteName = TEXT("First Blood");

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	int32 Grade = 4;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	int32 TrueGrade = 4;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	EDirtbagRouteType RouteType = EDirtbagRouteType::Power;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	FDirtbagClimber ClimberStats;

	// --- Staging dials (feel, not sim — sim dials live in Sim/) ---------

	/** Seconds at a hold before a sure move; risk stretches it. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float BaseHesitation = 0.4f;

	/** Extra seconds of hesitation at odds 0 — a 40% move visibly gathers. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float HesitationPerRisk = 1.5f;

	/** Seconds a hold-to-hold move takes. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float MoveDuration = 0.6f;

	/** Seconds from peel-off to the mat. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float FallDuration = 0.7f;

	/** Seconds the result card lingers before the camera hands back. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float EndPause = 2.0f;

	/** Hours of brushing per press of the clean key. Half an hour is small
	 *  enough that cleaning a line is several deliberate presses rather than
	 *  one, which is roughly what it feels like. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Verb")
	float CleanHoursPerPress = 0.5f;

	// --- The HOLD TO CLIMB verb (interactive mode; presentation dials — the
	// --- sim only ever sees the resulting 0..1 execution scalar) -----------

	/** Seconds of holding Space for the grip meter to reach full. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float ChargeTime = 0.9f;

	/** Sweet window on the grip meter: release inside it to latch the move.
	 *  Release below the window = shake out instead; charge past 1.0 =
	 *  over-grip, the move fires itself with OvergripExecution. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float SweetWindowStart = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float SweetWindowEnd = 0.95f;

	/** Execution for a release dead-center in the window... */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float PerfectExecution = 0.95f;

	/** ...tapering to this at the window's edges. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float EdgeExecution = 0.6f;

	/** The death grip: what holding on too long costs you. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float OvergripExecution = 0.35f;

private:
	enum class EPhase : uint8 { Idle, Mounting, AtStance, Moving, Falling, Ending };

	UFUNCTION()
	void OnApproachBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                     bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnApproachEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnInteract();
	void OnClean();
	void OnHoldPressed();
	void OnHoldReleased();
	void StartAttempt();
	void BeginSessionBody();
	void ScheduleNextMove();
	void BeginMove();
	void CommitMove(double Execution);
	void StageMoveResult(bool bSuccess);
	void FinishLiveAttempt();
	void FinishAttempt();
	void EndSession();
	void UpdateHud();
	void PlayAnim(UAnimSequence* Anim, bool bLoop);
	FVector HoldLocation(int32 Index) const;

	// Central state when present; the wall then reads the board and commits
	// through it. Null in game-instance-less test maps → legacy standalone.
	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	FDirtbagRoute Route;
	FDirtbagSessionState Session;
	FDirtbagProjectMemory Memory;
	FDirtbagAttemptResult Current;

	// Interactive-session state: the live attempt the player is driving.
	dirtbag::Route SimRoute;
	dirtbag::LiveAttempt Live;
	bool bLiveSession = false;
	bool bCharging = false;
	float Charge = 0.0f;

	EPhase Phase = EPhase::Idle;
	int32 TimelineIndex = 0;
	int32 HoldIndex = 0;
	bool bReachLeft = false;
	bool bPlayerNear = false;
	bool bBoundInput = false;
	float MoveAlpha = 0.0f;
	FVector MoveFrom = FVector::ZeroVector;
	FVector MoveTo = FVector::ZeroVector;
	FTimerHandle PhaseTimer;
};

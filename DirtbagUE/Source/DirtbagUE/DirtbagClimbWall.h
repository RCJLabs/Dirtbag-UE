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

	UPROPERTY(EditAnywhere, Category = "Dirtbag")
	TObjectPtr<UStaticMesh> HoldMarkerMesh;

	// --- Route / climber config -----------------------------------------

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

private:
	enum class EPhase : uint8 { Idle, Hesitating, Moving, Falling, Ending };

	UFUNCTION()
	void OnApproachBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                     bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnApproachEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnInteract();
	void StartAttempt();
	void ScheduleNextMove();
	void BeginMove();
	void FinishAttempt();
	void EndSession();
	void PlayAnim(UAnimSequence* Anim, bool bLoop);
	FVector HoldLocation(int32 Index) const;

	FDirtbagRoute Route;
	FDirtbagSessionState Session;
	FDirtbagProjectMemory Memory;
	FDirtbagAttemptResult Current;

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

// A place where a day-verb happens: the cooler, the front desk, the van.
// Walk up, press E. The actor is only the doorbell — every consequence
// (cash, hunger, energy, the day advancing, the save) belongs to the sim.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DirtbagGameInstance.h"

#include "DirtbagDaySpot.generated.h"

class UBoxComponent;
class USceneComponent;
class UDirtbagGameInstance;

UENUM(BlueprintType)
enum class EDirtbagSpotKind : uint8
{
	/** Gas-station burrito economics: costs cash, buys back hunger. */
	Meal,
	/** The belay-desk shift: pays, and takes the hours and the energy. */
	Shift,
	/** Lights out — advances the day and writes the save. */
	Sleep,
	/** The drive: half an hour of the day, and you are somewhere else. */
	Travel,
	/** Sitting it out — the log by the pads, the tailgate. Passes time, and
	 *  outdoors will wait exactly until the rock comes good, which is the
	 *  verb the whole conditions system was missing. */
	Rest,
	/** The fire at the Lot. Rest, with people: the hours pass the same way
	 *  and you get the company for them — rapport, and what everyone is
	 *  working this week. */
	Fire,
	/** The bowl by the van. Feed the stray until it is not a stray. */
	Dog
};

UCLASS()
class ADirtbagDaySpot : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagDaySpot();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagSpotKind Kind = EDirtbagSpotKind::Meal;

	/** Travel only: the spot at the other end of the drive. Point the van's
	 *  spot at the gym's and vice versa. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dirtbag|Travel")
	TObjectPtr<AActor> TravelTarget;

	/** Where this drive goes, for the prompt: "Drive to the gym?" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	FString TravelName = TEXT("the gym");

	/** Hours the drive eats. The van is not fast and the crag is not close. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	double TravelHours = 0.5;

	/** Where this drive lands you. Set on arrival, so the HUD's conditions
	 *  line is right from the moment you step out of the van rather than
	 *  from whenever you first touch a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	EDirtbagVenue ArriveAt = EDirtbagVenue::Gym;

	/** Rest only. How long one press sits for, when there is no window to
	 *  wait for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Rest")
	double RestHours = 1.0;

	/** Rest only. With a window later today, one press waits exactly until
	 *  it opens rather than an hour at a time. Turn it off to sit in fixed
	 *  chunks and watch the forecast change under you. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Rest")
	bool bWaitForWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	float FadeSeconds = 0.4f;

	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnInteract();
	void BeginDrive();
	void ArriveFromDrive();
	FString PromptText() const;

	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	bool bPlayerNear = false;
	bool bBoundInput = false;
	FTimerHandle DriveTimer;
};

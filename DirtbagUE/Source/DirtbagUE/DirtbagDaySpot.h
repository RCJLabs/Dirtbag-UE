// A place where a day-verb happens: the cooler, the front desk, the van.
// Walk up, press E. The actor is only the doorbell — every consequence
// (cash, hunger, energy, the day advancing, the save) belongs to the sim.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

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
	Travel
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

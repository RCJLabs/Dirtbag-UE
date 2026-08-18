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
	Sleep
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
	FString PromptText() const;

	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	bool bPlayerNear = false;
	bool bBoundInput = false;
};

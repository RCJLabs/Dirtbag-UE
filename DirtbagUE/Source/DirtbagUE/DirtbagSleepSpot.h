// The end of the day as a place: walk to the van (or wherever this actor
// sits), press E, sleep. The sim advances the day and the save file writes —
// this actor is only the doorbell.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DirtbagSleepSpot.generated.h"

class UBoxComponent;
class UDirtbagGameInstance;

UCLASS()
class ADirtbagSleepSpot : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagSleepSpot();

protected:
	virtual void BeginPlay() override;

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

	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	bool bPlayerNear = false;
	bool bBoundInput = false;
};

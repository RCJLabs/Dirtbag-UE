// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DirtbagUEPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ADirtbagUEPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Creation's six keys.
	 *
	 *  Every other numbered question in the game is asked at a counter, and
	 *  the counter binds its own keys when you walk into its trigger. The
	 *  four questions that build a climber are asked on the first frame of a
	 *  career, when the player is not standing at anything and may not have
	 *  a counter within a hundred metres -- so there was nothing listening
	 *  and the screen could not be answered at all. The player controller is
	 *  the one listener that is always there. It only spends a key while
	 *  creation is up; otherwise the press falls through to whatever else is
	 *  bound to it. */
	void OnCreationKey(int32 Which);
	void OnCreation1();
	void OnCreation2();
	void OnCreation3();
	void OnCreation4();
	void OnCreation5();
	void OnCreation6();

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};

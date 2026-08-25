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

	/** The keys belonging to a screen that owns the whole keyboard.
	 *
	 *  Every other question in the game is asked at a counter, and the
	 *  counter binds its own keys when you walk into its trigger. The four
	 *  questions that build a climber are asked on the first frame of a
	 *  career, when the player is not standing at anything and may not have
	 *  a counter within a hundred metres -- so there was nothing listening
	 *  and the screen could not be answered at all. Neither could its last
	 *  page, which says "E to get on with it" and meant it about a key
	 *  nobody was reading.
	 *
	 *  The player controller is the one listener that is always there. It
	 *  only spends a key while a screen is up; otherwise the press falls
	 *  through to whatever else is bound to it. What counts as a screen, and
	 *  what a key does on one, both live on the game instance -- so this and
	 *  the counters cannot drift apart. */
	void OnScreenKey(int32 Which);
	void OnScreenEnter();
	void OnScreen1();
	void OnScreen2();
	void OnScreen3();
	void OnScreen4();
	void OnScreen5();
	void OnScreen6();

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};

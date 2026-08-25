// Copyright Epic Games, Inc. All Rights Reserved.


#include "DirtbagUEPlayerController.h"
#include "Components/InputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "DirtbagUE.h"
#include "DirtbagGameInstance.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ADirtbagUEPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogDirtbagUE, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ADirtbagUEPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// See OnCreationKey. These are bound here and not at a counter
		// because creation is up before the player has walked anywhere.
		//
		// **Never consuming.** These six keys already belong to every
		// counter in the game, and where the player controller's own input
		// component sits in the engine's stack relative to an actor that
		// has called EnableInput is not a thing this file should be betting
		// on. A consuming binding here that happened to sort above the gym
		// counter would eat the levers -- a bug that only shows up on the
		// one machine that has an editor. So it listens and never
		// swallows, and UDirtbagGameInstance::LastCreationFrame is what
		// stops one press being answered twice.
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::One, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnCreation1)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Two, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnCreation2)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Three, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnCreation3)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Four, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnCreation4)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Five, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnCreation5)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Six, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnCreation6)
			    .bConsumeInput = false;
		}
	}
}

void ADirtbagUEPlayerController::OnCreationKey(int32 Which)
{
	if (UDirtbagGameInstance* Game =
	        Cast<UDirtbagGameInstance>(GetGameInstance()))
	{
		Game->ChooseInCreation(Which);
	}
}

void ADirtbagUEPlayerController::OnCreation1() { OnCreationKey(0); }
void ADirtbagUEPlayerController::OnCreation2() { OnCreationKey(1); }
void ADirtbagUEPlayerController::OnCreation3() { OnCreationKey(2); }
void ADirtbagUEPlayerController::OnCreation4() { OnCreationKey(3); }
void ADirtbagUEPlayerController::OnCreation5() { OnCreationKey(4); }
void ADirtbagUEPlayerController::OnCreation6() { OnCreationKey(5); }

bool ADirtbagUEPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

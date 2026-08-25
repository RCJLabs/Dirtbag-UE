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

		// See OnScreenKey. These are bound here and not at a counter
		// because a screen can be up before the player has walked
		// anywhere -- creation is, on the first frame of a career.
		//
		// **Never consuming.** These seven keys already belong to every
		// counter in the game -- E most of all, which is every spot's yes.
		// Where the player controller's own input component sits in the
		// engine's stack relative to an actor that has called EnableInput
		// is not a thing this file should be betting on: a consuming
		// binding here that happened to sort above the gym counter would
		// eat the levers, and above any counter at all would eat E, which
		// is a bug that only shows up on the one machine that has an
		// editor. So it listens and never swallows, and
		// UDirtbagGameInstance::LastScreenFrame is what stops one press
		// being answered twice.
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::E, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreenEnter)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::One, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreen1)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Two, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreen2)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Three, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreen3)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Four, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreen4)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Five, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreen5)
			    .bConsumeInput = false;
			InputComponent->BindKey(EKeys::Six, IE_Pressed, this,
			                        &ADirtbagUEPlayerController::OnScreen6)
			    .bConsumeInput = false;
		}
	}
}

void ADirtbagUEPlayerController::OnScreenKey(int32 Which)
{
	if (UDirtbagGameInstance* Game =
	        Cast<UDirtbagGameInstance>(GetGameInstance()))
	{
		Game->ChooseOnAScreen(Which);
	}
}

void ADirtbagUEPlayerController::OnScreenEnter()
{
	if (UDirtbagGameInstance* Game =
	        Cast<UDirtbagGameInstance>(GetGameInstance()))
	{
		Game->PressOnAScreen();
	}
}

void ADirtbagUEPlayerController::OnScreen1() { OnScreenKey(0); }
void ADirtbagUEPlayerController::OnScreen2() { OnScreenKey(1); }
void ADirtbagUEPlayerController::OnScreen3() { OnScreenKey(2); }
void ADirtbagUEPlayerController::OnScreen4() { OnScreenKey(3); }
void ADirtbagUEPlayerController::OnScreen5() { OnScreenKey(4); }
void ADirtbagUEPlayerController::OnScreen6() { OnScreenKey(5); }

bool ADirtbagUEPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

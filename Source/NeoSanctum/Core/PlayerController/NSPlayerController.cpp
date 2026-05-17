// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "NeoSanctum/Input/NSInputComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"

ANSPlayerController::ANSPlayerController()
{
}

void ANSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!InputConfig)
	{
		return;
	}

	UNSInputComponent* NSInputComponent = Cast<UNSInputComponent>(InputComponent);
	if (!NSInputComponent)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		NSInputComponent->AddInputMappingRoute(InputConfig, DefaultInputRoute, InputSubsystem);
		CurrentInputRoute = DefaultInputRoute;
	}
}

void ANSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UNSInputComponent* NSInputComponent = CastChecked<UNSInputComponent>(InputComponent);
	if (!IsValid(NSInputComponent))
	{
		return;
	}

	if (InputConfig)
	{
		NSInputComponent->BindNativeAction(
			InputConfig,
			NSGameplayTags::Input_Native_Move,
			ETriggerEvent::Triggered,
			this,
			&ThisClass::Input_Move,
			true
		);

		NSInputComponent->BindNativeAction(
			InputConfig,
			NSGameplayTags::Input_Native_Look,
			ETriggerEvent::Triggered,
			this,
			&ThisClass::Input_Look,
			true
		);

		TArray<uint32> BindHandles;
		NSInputComponent->BindAbilityActions(
			InputConfig,
			this,
			&ThisClass::Input_AbilityPressed,
			&ThisClass::Input_AbilityReleased,
			BindHandles
		);
	}
}

void ANSPlayerController::Input_Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector2D MoveValue = Value.Get<FVector2D>();
	const FRotator MoveRotation(0.f, GetControlRotation().Yaw, 0.f);

	const FVector ForwardDirection = MoveRotation.RotateVector(FVector::ForwardVector);
	const FVector RightDirection = MoveRotation.RotateVector(FVector::RightVector);

	ControlledPawn->AddMovementInput(ForwardDirection, MoveValue.Y);
	ControlledPawn->AddMovementInput(RightDirection, MoveValue.X);
}

void ANSPlayerController::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();

	AddYawInput(LookValue.X);
	AddPitchInput(LookValue.Y);
}

void ANSPlayerController::Input_AbilityPressed(FGameplayTag InputTag)
{
	// ASC 추가 이후 구현
}

void ANSPlayerController::Input_AbilityReleased(FGameplayTag InputTag)
{
	// ASC 추가 이후 구현
}

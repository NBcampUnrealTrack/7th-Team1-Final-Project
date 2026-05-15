// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerController.h"

#include "NeoSanctum/Input/NSInputComponent.h"

ANSPlayerController::ANSPlayerController()
{
}

void ANSPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ANSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UNSInputComponent* NSInputComponent = CastChecked<UNSInputComponent>(InputComponent);
	if (!IsValid(NSInputComponent)) return;
	
	if (InputConfig)
	{
		// NativeAction 바인딩하는 부분
	}
}

void ANSPlayerController::Input_Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;
	
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
	
}

void ANSPlayerController::Input_AbilityReleased(FGameplayTag InputTag)
{
	
}

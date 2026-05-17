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

	SetInputConfig(DefaultInputConfig, DefaultInputRoute);
}

void ANSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BindInputActions();
}

void ANSPlayerController::BindInputActions()
{
	UNSInputComponent* NSInputComponent = Cast<UNSInputComponent>(InputComponent);
	if (!NSInputComponent || !CurrentInputConfig)
	{
		return;
	}

	UnbindInputActions();

	NSInputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Move,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::Input_Move,
		true,
		NativeInputBindHandles
	);

	NSInputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Look,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::Input_Look,
		true,
		NativeInputBindHandles
	);

	NSInputComponent->BindAbilityActions(
		CurrentInputConfig,
		this,
		&ThisClass::Input_AbilityPressed,
		&ThisClass::Input_AbilityReleased,
		AbilityInputBindHandles
	);
}

void ANSPlayerController::UnbindInputActions()
{
	if (UNSInputComponent* NSInputComponent = Cast<UNSInputComponent>(InputComponent))
	{
		NSInputComponent->RemoveBinds(NativeInputBindHandles);
		NSInputComponent->RemoveBinds(AbilityInputBindHandles);
	}
}

void ANSPlayerController::SetInputRoute(ENSInputRoute NewInputRoute)
{
	SetInputConfig(CurrentInputConfig, NewInputRoute);
}

void ANSPlayerController::SetInputConfig(UNSInputConfig* NewInputConfig, ENSInputRoute NewInputRoute)
{
	if (!IsLocalController())
	{
		return;
	}

	UNSInputComponent* NSInputComponent = Cast<UNSInputComponent>(InputComponent);
	if (!NSInputComponent)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!InputSubsystem)
	{
		return;
	}

	if (bHasAppliedInputConfig && CurrentInputConfig == NewInputConfig && CurrentInputRoute == NewInputRoute)
	{
		return;
	}

	if (NewInputConfig && !NewInputConfig->FindInputRoute(NewInputRoute))
	{
		return;
	}

	if (bHasAppliedInputConfig && CurrentInputConfig)
	{
		NSInputComponent->RemoveInputMappingRoute(CurrentInputConfig, CurrentInputRoute, InputSubsystem);
	}

	UnbindInputActions();

	CurrentInputConfig = NewInputConfig;
	CurrentInputRoute = NewInputRoute;

	if (!CurrentInputConfig)
	{
		bHasAppliedInputConfig = false;
		return;
	}

	NSInputComponent->AddInputMappingRoute(CurrentInputConfig, CurrentInputRoute, InputSubsystem);
	BindInputActions();
	bHasAppliedInputConfig = true;
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

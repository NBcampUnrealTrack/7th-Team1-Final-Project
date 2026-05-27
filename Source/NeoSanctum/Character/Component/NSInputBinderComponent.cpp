// Copyright 2026 One Team. All rights reserved.

#include "NSInputBinderComponent.h"

#include "AbilitySystemInterface.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Input/NSInputComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"

UNSInputBinderComponent::UNSInputBinderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	DefaultInputModeTags.AddTag(NSGameplayTags::InputMode_Gameplay);
	DefaultInputModeTags.AddTag(NSGameplayTags::InputMode_UI);
	ActiveInputModeTags = DefaultInputModeTags;
}

void UNSInputBinderComponent::BeginPlay()
{
	Super::BeginPlay();

	SetInputConfig(DefaultInputConfig);
}

void UNSInputBinderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInputConfig();
	UnbindInputActions();

	Super::EndPlay(EndPlayReason);
}

void UNSInputBinderComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	InputComponent = Cast<UNSInputComponent>(PlayerInputComponent);
	ApplyInputConfig();
}

void UNSInputBinderComponent::SetInputConfig(UNSInputConfig* NewConfig)
{
	if (bHasAppliedInputConfig && CurrentInputConfig == NewConfig)
	{
		return;
	}

	RemoveInputConfig();
	UnbindInputActions();

	CurrentInputConfig = NewConfig;

	ApplyInputConfig();
}

void UNSInputBinderComponent::SetActiveInputModeTags(const FGameplayTagContainer& NewInputModeTags)
{
	if (ActiveInputModeTags == NewInputModeTags)
	{
		return;
	}

	const bool bShouldReapplyInputConfig = bHasAppliedInputConfig;
	if (bShouldReapplyInputConfig)
	{
		RemoveInputConfig();
	}

	ActiveInputModeTags = NewInputModeTags;

	if (bShouldReapplyInputConfig)
	{
		ApplyInputConfig();
	}
}

void UNSInputBinderComponent::ApplyInputConfig()
{
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();
	if (!InputComponent || !InputSubsystem || !CurrentInputConfig || bHasAppliedInputConfig)
	{
		return;
	}

	InputComponent->AddInputMappings(CurrentInputConfig, InputSubsystem, ActiveInputModeTags);
	BindInputActions();
	bHasAppliedInputConfig = true;
}

void UNSInputBinderComponent::RemoveInputConfig()
{
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();
	if (!InputComponent || !InputSubsystem || !CurrentInputConfig || !bHasAppliedInputConfig)
	{
		bHasAppliedInputConfig = false;
		return;
	}

	InputComponent->RemoveInputMappings(CurrentInputConfig, InputSubsystem, ActiveInputModeTags);
	bHasAppliedInputConfig = false;
}

void UNSInputBinderComponent::BindInputActions()
{
	if (!InputComponent || !CurrentInputConfig)
	{
		return;
	}

	UnbindInputActions();

	InputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Move,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::Input_Move,
		true,
		NativeInputBindHandles
	);

	InputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Look,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::Input_Look,
		true,
		NativeInputBindHandles
	);

	InputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Jump,
		ETriggerEvent::Triggered,
		this,
		&ThisClass::Input_Jump,
		true,
		NativeInputBindHandles
	);

	InputComponent->BindAbilityActions(
		CurrentInputConfig,
		this,
		&ThisClass::Input_AbilityPressed,
		&ThisClass::Input_AbilityReleased,
		AbilityInputBindHandles
	);
}

void UNSInputBinderComponent::UnbindInputActions()
{
	if (InputComponent)
	{
		InputComponent->RemoveBinds(NativeInputBindHandles);
		InputComponent->RemoveBinds(AbilityInputBindHandles);
	}
}

void UNSInputBinderComponent::Input_Move(const FInputActionValue& Value)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	const AController* Controller = OwnerPawn->GetController();
	if (!Controller)
	{
		return;
	}

	const FVector2D MoveValue = Value.Get<FVector2D>();
	const FRotator MoveRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	const FVector ForwardDirection = MoveRotation.RotateVector(FVector::ForwardVector);
	const FVector RightDirection = MoveRotation.RotateVector(FVector::RightVector);

	OwnerPawn->AddMovementInput(ForwardDirection, MoveValue.Y);
	OwnerPawn->AddMovementInput(RightDirection, MoveValue.X);
}

void UNSInputBinderComponent::Input_Look(const FInputActionValue& Value)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	const FVector2D LookValue = Value.Get<FVector2D>();

	PlayerController->AddYawInput(LookValue.X);
	PlayerController->AddPitchInput(LookValue.Y);
}

void UNSInputBinderComponent::Input_Jump()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->Jump();
	}
}

void UNSInputBinderComponent::Input_AbilityPressed(FGameplayTag InputTag)
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UNSAbilitySystemComponent* ASC = Cast<UNSAbilitySystemComponent>(ASI->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagPressed(InputTag);
		}
	}
}

void UNSInputBinderComponent::Input_AbilityReleased(FGameplayTag InputTag)
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UNSAbilitySystemComponent* ASC = Cast<UNSAbilitySystemComponent>(ASI->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagReleased(InputTag);
		}
	}
}

UEnhancedInputLocalPlayerSubsystem* UNSInputBinderComponent::GetInputSubsystem() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;

	return LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
}

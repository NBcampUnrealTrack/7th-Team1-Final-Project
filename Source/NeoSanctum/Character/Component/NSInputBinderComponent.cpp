// Copyright 2026 One Team. All rights reserved.

#include "NSInputBinderComponent.h"

#include "AbilitySystemInterface.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "CommonInputSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Input/NSInputComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/Options/NSUISettingsSubsystem.h"
#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"

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

	InputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Death_PrevPlayer,
		ETriggerEvent::Started,
		this,
		&ThisClass::Input_SpectatePrevious,
		true,
		NativeInputBindHandles
	);

	InputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Death_NextPlayer,
		ETriggerEvent::Started,
		this,
		&ThisClass::Input_SpectateNext,
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
	
	InputComponent->BindAugmentActions(
		CurrentInputConfig,
		this,
		&ThisClass::Input_AugmentAction,
		AugmentInputBindHandles
	);
	
	InputComponent->BindNativeAction(
		CurrentInputConfig,
		NSGameplayTags::Input_Native_Interact,
		ETriggerEvent::Started,
		this,
		&ThisClass::Input_Interact,
		true,
		NativeInputBindHandles
	);
}

void UNSInputBinderComponent::UnbindInputActions()
{
	if (InputComponent)
	{
		InputComponent->RemoveBinds(NativeInputBindHandles);
		InputComponent->RemoveBinds(AbilityInputBindHandles);
		InputComponent->RemoveBinds(AugmentInputBindHandles);
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

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerPawn))
	{
		if (const UNSAbilitySystemComponent* ASC = Cast<UNSAbilitySystemComponent>(ASI->GetAbilitySystemComponent()))
		{
			if (ASC->HasMatchingGameplayTag(NSGameplayTags::State_Input_BlockInputMove))
			{
				return;
			}
		}
	}

	const FVector2D MoveValue = Value.Get<FVector2D>();
	const FRotator MoveRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	const FVector ForwardDirection = MoveRotation.RotateVector(FVector::ForwardVector);
	const FVector RightDirection = MoveRotation.RotateVector(FVector::RightVector);

	OwnerPawn->AddMovementInput(ForwardDirection, MoveValue.Y);
	OwnerPawn->AddMovementInput(RightDirection, MoveValue.X);

	// 아웃런 조작법 안내 — 이동 입력 발동 알림 (인런 등 다른 월드에서는 서브시스템 내부 게이트로 무시됨)
	if (!MoveValue.IsNearlyZero())
	{
		if (UNSOutRunGuideSubsystem* GuideSubsystem =
			GetWorld()->GetSubsystem<UNSOutRunGuideSubsystem>())
		{
			GuideSubsystem->NotifyMoveInput();
		}
	}
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

	float LookScale = 1.0f;

	ULocalPlayer* LocalPlayer =
		PlayerController->GetLocalPlayer();

	const UCommonInputSubsystem* CommonInputSubsystem =
		LocalPlayer
			? ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(
				LocalPlayer)
			: nullptr;

	if (CommonInputSubsystem &&
		CommonInputSubsystem->GetCurrentInputType() ==
			ECommonInputType::MouseAndKeyboard)
	{
		const UGameInstance* GameInstance =
			PlayerController->GetGameInstance();

		const UNSUISettingsSubsystem* Settings =
			GameInstance
				? GameInstance->GetSubsystem<
					UNSUISettingsSubsystem>()
				: nullptr;

		if (Settings)
		{
			LookScale =
				Settings->GetMouseSensitivity();
		}
	}

	PlayerController->AddYawInput(
		LookValue.X * LookScale);

	PlayerController->AddPitchInput(
		LookValue.Y * LookScale);
}

void UNSInputBinderComponent::Input_Jump()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		// 아웃런 조작법 안내 — 점프 입력 발동 알림
		if (UNSOutRunGuideSubsystem* GuideSubsystem =
			GetWorld()->GetSubsystem<UNSOutRunGuideSubsystem>())
		{
			GuideSubsystem->NotifyJumpInput();
		}

		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Character))
		{
			if (UNSAbilitySystemComponent* ASC = Cast<UNSAbilitySystemComponent>(ASI->GetAbilitySystemComponent()))
			{
				FGameplayTagContainer ParkourAbilityTags;
				ParkourAbilityTags.AddTag(NSGameplayTags::Ability_Common_Parkour);
				if (ASC->TryActivateAbilitiesByTag(ParkourAbilityTags))
				{
					return;
				}
			}
		}

		Character->Jump();
	}
}

void UNSInputBinderComponent::Input_SpectatePrevious()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	ANSPlayerController* PlayerController = OwnerPawn ? Cast<ANSPlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PlayerController)
	{
		PlayerController->SpectatePreviousPlayer();
	}
}

void UNSInputBinderComponent::Input_SpectateNext()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	ANSPlayerController* PlayerController = OwnerPawn ? Cast<ANSPlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PlayerController)
	{
		PlayerController->SpectateNextPlayer();
	}
}

void UNSInputBinderComponent::Input_AugmentAction(FGameplayTag InputTag)
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (!GameInstance)
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}
	
	if (InputTag == NSGameplayTags::Input_Augment_TogglePanel)
	{
		const APawn* OwnerPawn = Cast<APawn>(GetOwner());
		ANSPlayerController* PlayerController = OwnerPawn ? Cast<ANSPlayerController>(OwnerPawn->GetController()) : nullptr;
		if (PlayerController)
		{
			PlayerController->ToggleAugmentationPanel();
		}
		return;
	}
	
	if (InputTag == NSGameplayTags::Input_Augment_TogglePartInventory)
	{
		const APawn* OwnerPawn = Cast<APawn>(GetOwner());
		ANSPlayerController* PlayerController =
			OwnerPawn
				? Cast<ANSPlayerController>(OwnerPawn->GetController())
				: nullptr;

		if (PlayerController)
		{
			PlayerController->TogglePartInventoryPanel();
		}

		return;
	}
	
	// 증강 패널이 닫혀있으면 1/2/3/T 입력 무시
	if (!UIManager->IsAugmentationPanelOpen())
	{
		return;
	}

	if (InputTag == NSGameplayTags::Input_Augment_Reroll)
	{
		UIManager->RequestRerollAugment();
		return;
	}

	// 카드 선택 -> 태그로 인덱스 판정
	int32 CardIndex = INDEX_NONE;
	if (InputTag == NSGameplayTags::Input_Augment_Card1)
	{
		CardIndex = 0;
	}
	else if (InputTag == NSGameplayTags::Input_Augment_Card2)
	{
		CardIndex = 1;
	}
	else if (InputTag == NSGameplayTags::Input_Augment_Card3)
	{
		CardIndex = 2;
	}
	else if (InputTag == NSGameplayTags::Input_Augment_Card4)
	{
		CardIndex = 3;
	}

	if (CardIndex == INDEX_NONE)
	{
		return;
	}

	UIManager->SelectAugmentCardByIndex(CardIndex);
}

void UNSInputBinderComponent::Input_Interact()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	ANSPlayerController* PlayerController =
		OwnerPawn ? Cast<ANSPlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PlayerController)
	{
		PlayerController->TryInteract();
	}
	
}

void UNSInputBinderComponent::Input_AbilityPressed(FGameplayTag InputTag)
{
	// 아웃런 조작법 안내 — 대시 입력 발동 알림
	if (InputTag == NSGameplayTags::Ability_Common_Dash)
	{
		if (UNSOutRunGuideSubsystem* GuideSubsystem =
			GetWorld()->GetSubsystem<UNSOutRunGuideSubsystem>())
		{
			GuideSubsystem->NotifyDashInput();
		}
	}

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

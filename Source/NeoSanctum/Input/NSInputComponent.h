// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"
#include "NSInputConfig.h"
#include "NSInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	UNSInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(
		const UNSInputConfig* InputConfig,
		UEnhancedInputLocalPlayerSubsystem* InputSubSystem,
		const FGameplayTag& InputModeTag = FGameplayTag()
	);

	void RemoveInputMappings(
		const UNSInputConfig* InputConfig,
		UEnhancedInputLocalPlayerSubsystem* InputSubSystem,
		const FGameplayTag& InputModeTag = FGameplayTag()
	) const;

	void RemoveBinds(TArray<uint32>& BindHandles);

public:
	template <class UserClass, typename FuncType>
	void BindNativeAction(
		const UNSInputConfig* InputConfig,
		const FGameplayTag& InputTag,
		ETriggerEvent TriggerEvent,
		UserClass* Object,
		FuncType Func,
		bool bLogIfNotFound,
		TArray<uint32>& BindHandles
	);

	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(
		const UNSInputConfig* InputConfig,
		UserClass* Object,
		PressedFuncType PressedFunc,
		ReleasedFuncType ReleasedFunc,
		TArray<uint32>& BindHandles
	);
};

template <class UserClass, typename FuncType>
void UNSInputComponent::BindNativeAction(
	const UNSInputConfig* InputConfig,
	const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent,
	UserClass* Object, FuncType Func,
	bool bLogIfNotFound,
	TArray<uint32>& BindHandles
)
{
	check(InputConfig);

	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindHandles.Add(BindAction(IA, TriggerEvent, Object, Func).GetHandle());
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UNSInputComponent::BindAbilityActions(
	const UNSInputConfig* InputConfig,
	UserClass* Object,
	PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
	TArray<uint32>& BindHandles
)
{
	check(InputConfig);

	for (const FNSInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(
					Action.InputAction,
					Action.PressedTriggerEvent,
					Object,
					PressedFunc,
					Action.InputTag
				).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(
					Action.InputAction,
					Action.ReleasedTriggerEvent,
					Object,
					ReleasedFunc,
					Action.InputTag
				).GetHandle());
			}
		}
	}
}

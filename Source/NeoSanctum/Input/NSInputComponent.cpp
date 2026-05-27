// Copyright 2026 One Team. All rights reserved.

#include "NSInputComponent.h"

#include "EnhancedInputSubsystems.h"

UNSInputComponent::UNSInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UNSInputComponent::AddInputMappings(
	const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem,
	const FGameplayTag& InputModeTag
)
{
	check(InputConfig);
	check(InputSubSystem);

	for (const FNSInputMappingContext& MappingContext : InputConfig->MappingContexts)
	{
		if (!MappingContext.InputMappingContext)
		{
			continue;
		}

		// InputMode 태그가 존재하는데 파라미터의 태그와 다른 태그라면 추가하지 않고 스킵
		if (InputModeTag.IsValid() && MappingContext.InputModeTag != InputModeTag)
		{
			continue;
		}

		// InputMode 태그가 존재하지 않으면 모두 추가 / 존재하면 위 if문에서 걸러져서 해당 InputTag만 추가됨
		InputSubSystem->AddMappingContext(MappingContext.InputMappingContext, MappingContext.Priority);
	}
}

void UNSInputComponent::RemoveInputMappings(
	const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem,
	const FGameplayTag& InputModeTag
) const
{
	check(InputConfig);
	check(InputSubSystem);

	for (const FNSInputMappingContext& MappingContext : InputConfig->MappingContexts)
	{
		if (!MappingContext.InputMappingContext)
		{
			continue;
		}

		// InputMode 태그가 존재하는데 파라미터의 태그와 다른 태그라면 제거하지 않고 스킵
		if (InputModeTag.IsValid() && MappingContext.InputModeTag != InputModeTag)
		{
			continue;
		}

		// InputMode 태그가 존재하지 않으면 모두 제거 / 존재하면 위 if문에서 걸러져서 해당 InputTag만 제거됨
		InputSubSystem->RemoveMappingContext(MappingContext.InputMappingContext);
	}
}

void UNSInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}

	BindHandles.Reset();
}

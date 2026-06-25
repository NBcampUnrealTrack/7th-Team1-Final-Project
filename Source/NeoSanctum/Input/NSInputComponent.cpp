// Copyright 2026 One Team. All rights reserved.

#include "NSInputComponent.h"

#include "EnhancedInputSubsystems.h"

UNSInputComponent::UNSInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UNSInputComponent::AddInputMappings(
	const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem,
	const FGameplayTagContainer& InputModeTags
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

		// InputMode 태그 묶음이 존재하면 해당 모드에 포함된 MappingContext만 추가
		if (!InputModeTags.IsEmpty()
			&& (!MappingContext.InputModeTag.IsValid() || !InputModeTags.HasTagExact(MappingContext.InputModeTag)))
		{
			continue;
		}

		// InputMode 태그 묶음이 비어있으면 기존 동작처럼 모두 추가
		InputSubSystem->AddMappingContext(MappingContext.InputMappingContext, MappingContext.Priority);
	}
}

void UNSInputComponent::RemoveInputMappings(
	const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem,
	const FGameplayTagContainer& InputModeTags
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

		// InputMode 태그 묶음이 존재하면 해당 모드에 포함된 MappingContext만 제거
		if (!InputModeTags.IsEmpty()
			&& (!MappingContext.InputModeTag.IsValid() || !InputModeTags.HasTagExact(MappingContext.InputModeTag)))
		{
			continue;
		}

		// InputMode 태그 묶음이 비어있으면 기존 동작처럼 모두 제거
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

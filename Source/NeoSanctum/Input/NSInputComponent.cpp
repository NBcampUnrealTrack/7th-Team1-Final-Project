// Copyright 2026 One Team. All rights reserved.

#include "NSInputComponent.h"

#include "EnhancedInputSubsystems.h"

UNSInputComponent::UNSInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UNSInputComponent::AddInputMappings(
	const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem)
{
	check(InputConfig);
	check(InputSubSystem);

	for (const FNSInputMappingContext& MappingContext : InputConfig->MappingContexts)
	{
		if (MappingContext.InputMappingContext)
		{
			InputSubSystem->AddMappingContext(MappingContext.InputMappingContext, MappingContext.Priority);
		}
	}
}

void UNSInputComponent::RemoveInputMappings(
	const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem) const
{
	check(InputConfig);
	check(InputSubSystem);

	for (const FNSInputMappingContext& MappingContext : InputConfig->MappingContexts)
	{
		if (MappingContext.InputMappingContext)
		{
			InputSubSystem->RemoveMappingContext(MappingContext.InputMappingContext);
		}
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

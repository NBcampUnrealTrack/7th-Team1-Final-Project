// Copyright 2026 One Team. All rights reserved.

#include "NSInputComponent.h"

UNSInputComponent::UNSInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UNSInputComponent::AddInputMapping(const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem)
{
	check(InputConfig);
	check(InputSubSystem);
}

void UNSInputComponent::RemoveInputMapping(const UNSInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem) const
{
	check(InputConfig);
	check(InputSubSystem);
}

void UNSInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	
	BindHandles.Reset();
}

// Copyright 2026 One Team. All rights reserved.

#include "NSInputComponent.h"

#include "EnhancedInputSubsystems.h"

UNSInputComponent::UNSInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UNSInputComponent::AddInputMappingRoute(
	const UNSInputConfig* InputConfig,
	ENSInputRoute InputRoute,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem)
{
	check(InputConfig);
	check(InputSubSystem);

	const FNSInputRoute* FoundRoute = InputConfig->FindInputRoute(InputRoute);
	if (!FoundRoute)
	{
		return;
	}

	for (const FNSInputMappingContext& MappingContext : FoundRoute->MappingContexts)
	{
		if (MappingContext.InputMappingContext)
		{
			InputSubSystem->AddMappingContext(MappingContext.InputMappingContext, MappingContext.Priority);
		}
	}
}

void UNSInputComponent::RemoveInputMappingRoute(
	const UNSInputConfig* InputConfig,
	ENSInputRoute InputRoute,
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem) const
{
	check(InputConfig);
	check(InputSubSystem);

	const FNSInputRoute* FoundRoute = InputConfig->FindInputRoute(InputRoute);
	if (!FoundRoute)
	{
		return;
	}

	for (const FNSInputMappingContext& MappingContext : FoundRoute->MappingContexts)
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

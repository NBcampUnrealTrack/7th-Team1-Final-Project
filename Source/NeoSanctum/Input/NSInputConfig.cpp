// Copyright 2026 One Team. All rights reserved.

#include "NSInputConfig.h"

UNSInputConfig::UNSInputConfig(const FObjectInitializer& ObjectInitializer)
{
}

const UInputAction* UNSInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FNSInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}
	
	return nullptr;
}

const UInputAction* UNSInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FNSInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}


// Copyright 2026 One Team. All rights reserved.


#include "NSAbilitySystemComponent.h"

void UNSAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}
		
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}
		
		AbilitySpec.InputPressed = true;
		
		// 이미 활성화된 Ability라면 입력 이벤트만 전달
		if (AbilitySpec.IsActive())
		{
			AbilitySpecInputPressed(AbilitySpec);
			continue;
		}
		
		TryActivateAbility(AbilitySpec.Handle);
	}
}

void UNSAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}
		
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}
		
		AbilitySpec.InputPressed = false;
		
		if (AbilitySpec.IsActive())
		{
			AbilitySpecInputReleased(AbilitySpec);
		}
	}
}

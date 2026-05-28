// Copyright 2026 One Team. All rights reserved.


#include "NSAbilitySystemComponent.h"

void UNSAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	
	// ASC가 Ability를 순회하는 동안 목록 변경을 막는 스코프 락
	ABILITYLIST_SCOPE_LOCK();
	
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
		
		// 입력상태를 즉시 실행하지 않고 큐에 저장
		InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
		InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
	}
}

void UNSAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	
	// ASC가 Ability를 순회하는 동안 목록 변경을 막는 스코프 락
	ABILITYLIST_SCOPE_LOCK();
	
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
		
		InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
		InputHeldSpecHandles.Remove(AbilitySpec.Handle);
	}
}

void UNSAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (bGamePaused)
	{
		ClearAbilityInput();
		return;
	}
	
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();
	
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		
		if (!AbilitySpec || !AbilitySpec->Ability)
		{
			continue;
		}
		
		AbilitySpec->InputPressed = true;
		
		if (AbilitySpec->IsActive())
		{
			// 이미 실행 중인 Ability라면 입력 이벤트만 전달
			AbilitySpecInputPressed(*AbilitySpec);
			continue;
		}
		
		AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
	}
	
	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}
	
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		
		if (!AbilitySpec || !AbilitySpec->Ability)
		{
			continue;
		}
		
		AbilitySpec->InputPressed = false;
		
		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}
	
	// Held는 유지해야 하므로 Pressed/Released 이벤트만 비움
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UNSAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

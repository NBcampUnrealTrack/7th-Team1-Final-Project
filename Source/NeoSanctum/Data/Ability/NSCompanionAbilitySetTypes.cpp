// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionAbilitySetTypes.h"
#include "AbilitySystemComponent.h"

void FNSCompanionAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& SpecHandle)
{
	if(!SpecHandle.IsValid()) return;
	AbilitySpecHandles.Add(SpecHandle);
}

void FNSCompanionAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& EffectHandle)
{
	if (!EffectHandle.IsValid()) return;
	GameplayEffectHandles.Add(EffectHandle);
}

void FNSCompanionAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	
	if (!ASC->IsOwnerActorAuthoritative()) return;
	
	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitySpecHandles)
	{
		if (SpecHandle.IsValid())
		{
			ASC->ClearAbility(SpecHandle);
		}
	}
	
	for (const FActiveGameplayEffectHandle& EffectHandle : GameplayEffectHandles)
	{
		if (EffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(EffectHandle);
		}
	}
	
	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
}

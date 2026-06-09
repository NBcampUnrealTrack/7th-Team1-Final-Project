// Copyright 2026 One Team. All rights reserved.


#include "GA_CompanionBasicFire.h"

#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Companion.h"

UGA_CompanionBasicFire::UGA_CompanionBasicFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	AbilityTags.AddTag(NSGameplayTags::Ability_Companion_Fire);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Companion_Disable);
	
	DamageSetTag = NSGameplayTags::Data_Companion_Damage;
	CoolDownTag = NSGameplayTags::Data_Companion_CoolDown;
}

void UGA_CompanionBasicFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_CompanionBasicFire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

// Copyright 2026 One Team. All rights reserved.


#include "GA_SkillBase.h"

#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_SkillBase::UGA_SkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

bool UGA_SkillBase::TryGetBaseAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const UNSAbilitySystemComponent* NSASC =
		Cast<UNSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	
	if (!NSASC)
	{
		return false;
	}
	
	return NSASC->TryGetBaseAbilityStat(AbilityTag, StatTag, OutValue);
}

bool UGA_SkillBase::TryGetFinalAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const UNSAbilitySystemComponent* NSASC = 
		Cast<UNSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	
	if (!NSASC)
	{
		return false;
	}
	
	return NSASC->TryGetFinalAbilityStat(AbilityTag, StatTag, OutValue);
}

float UGA_SkillBase::GetFinalAbilityStatOrDefault(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float DefaultValue) const
{
	float Value = DefaultValue;
	
	TryGetFinalAbilityStat(AbilityTag, StatTag, Value);
	
	return Value;
}

float UGA_SkillBase::GetBaseAbilityStatOrDefault(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float DefaultValue) const
{
	float Value = DefaultValue;
	
	TryGetBaseAbilityStat(AbilityTag, StatTag, Value);
	
	return Value;
}

UNSAbilitySystemComponent* UGA_SkillBase::GetNSAbilitySystemComponent() const
{
	return Cast<UNSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

ANSPlayerState* UGA_SkillBase::GetNSPlayerState() const
{
	return Cast<ANSPlayerState>(GetOwningActorFromActorInfo());
}

FString UGA_SkillBase::GetCurrentPredictionKeyStatus()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return TEXT("NSAbilitySystemComponent 없음");
	}
	
	return ASC->ScopedPredictionKey.ToString() + TEXT("예측키가 아직 열려 있나: ")
		+ (ASC->ScopedPredictionKey.IsValidForMorePrediction() ? TEXT("열려있음") : TEXT("닫힘"));
}

bool UGA_SkillBase::IsPredictionKeyValidForMorePrediction() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}
	
	return ASC->ScopedPredictionKey.IsValidForMorePrediction();
}

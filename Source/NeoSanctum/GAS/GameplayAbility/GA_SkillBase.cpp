// Copyright 2026 One Team. All rights reserved.


#include "GA_SkillBase.h"

#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_SkillBase::UGA_SkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

UNSAbilitySystemComponent* UGA_SkillBase::GetNSASC() const
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
		return TEXT("NSASC 없음");
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

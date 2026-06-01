// Copyright 2026 One Team. All rights reserved.


#include "GA_Engineer_SpawnTurret.h"

#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"

UGA_Engineer_SpawnTurret::UGA_Engineer_SpawnTurret()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Engineer_SpawnTurret);
	SetAssetTags(AssetTags);
	
	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
}

void UGA_Engineer_SpawnTurret::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// cost 관련
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

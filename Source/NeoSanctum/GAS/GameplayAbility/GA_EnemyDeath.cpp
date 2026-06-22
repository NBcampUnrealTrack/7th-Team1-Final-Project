// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyDeath.h"

#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyDeath::UGA_EnemyDeath()
{
	// Advanced 세팅 설정
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;

	// Tags 세팅 설정
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::State_Dead);
	SetAssetTags(AssetTags);

	CancelAbilitiesWithTag.AddTag(NSGameplayTags::State_Enemy_Combat);
	CancelAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_Attack);
	CancelAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_HitReaction);

	BlockAbilitiesWithTag.AddTag(NSGameplayTags::State_Enemy_Combat);
	BlockAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_Attack);
	BlockAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_HitReaction);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Dead);
}

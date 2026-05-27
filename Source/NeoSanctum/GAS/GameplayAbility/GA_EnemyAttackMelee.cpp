// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackMelee.h"

#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyAttackMelee::UGA_EnemyAttackMelee()
{
	// Tags 세팅 설정
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_MeleeAttack);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);

	AttackTraceDistance = 100.0f;
	AttackTraceRadius = 80.0f;
}

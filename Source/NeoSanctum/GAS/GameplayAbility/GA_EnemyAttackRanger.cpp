// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackRanger.h"

#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyAttackRanger::UGA_EnemyAttackRanger()
{
	// Tags 세팅 설정
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_RangerAttack);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackRanger::InitializeAttack()
{
	CurrentShotCount = 0;
	
	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		AnimInstance->Montage_SetNextSection(
			TEXT("Fire"),
			TEXT("Fire"),
			AttackMontage);
	}
}

void UGA_EnemyAttackRanger::HandleAttackMontageCompleted()
{
	Super::HandleAttackMontageCompleted();
}

void UGA_EnemyAttackRanger::HandleAttackEvent(const FGameplayEventData& Payload)
{
	++CurrentShotCount;

	if (UAnimInstance* AnimInstance =
		GetActorInfo().GetAnimInstance())
	{
		const FName NextSection = CurrentShotCount <= BurstCount ? FName(TEXT("Fire")) : NAME_None;

		AnimInstance->Montage_SetNextSection(
			TEXT("Fire"),
			NextSection,
			AttackMontage);
	}
	
	// TODO: 투사체 생성
}

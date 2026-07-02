// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyHitReaction.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyHitReaction::UGA_EnemyHitReaction()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_HitReaction);
	SetAssetTags(AssetTags);

	// 경직 시작 시 진행 중인 몬스터 공격 Ability를 취소
	CancelAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_Attack);

	// 경직 중 새로운 공격 Ability 실행을 차단
	BlockAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_Attack);

	// 활성화된 동안 ASC에 경직 상태 태그를 부여
	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_HitReacting);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Enemy_HitReacting);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyHitReaction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UNSEnemyStateComponent* EnemyState =
		AvatarActor ? AvatarActor->FindComponentByClass<UNSEnemyStateComponent>() : nullptr;

	if (!EnemyState ||
		EnemyState->IsDead() ||
		EnemyState->IsInactive() ||
		!HitReactionMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::
		CreatePlayMontageAndWaitProxy(
			this,
			TEXT("HitReactionMontageTask"),
			HitReactionMontage,
			FMath::Max(MontagePlayRate, 0.01f),
			StartSectionName,
			true,
			1.0f,
			0.0f);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnHitReactionMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnHitReactionMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnHitReactionMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UGA_EnemyHitReaction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UNSEnemyStateComponent* EnemyState =
		AvatarActor ? AvatarActor->FindComponentByClass<UNSEnemyStateComponent>() : nullptr;

	if (EnemyState)
	{
		EnemyState->FinishHitReaction();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EnemyHitReaction::OnHitReactionMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyHitReaction::OnHitReactionMontageInterrupted()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

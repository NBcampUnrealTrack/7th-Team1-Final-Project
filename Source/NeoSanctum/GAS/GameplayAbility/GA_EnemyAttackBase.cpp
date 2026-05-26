// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGA_EnemyAttackBase::UGA_EnemyAttackBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_EnemyAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitCheckEventTag, nullptr, false, false);

	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UGA_EnemyAttackBase::OnHitCheckEventReceived);
		EventTask->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("AttackMontageTask"),
		AttackMontage,
		1.0f,
		NAME_None,
		false,
		1.0f,
		0.0f);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttackBase::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttackBase::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttackBase::OnMontageInterrupted);

		MontageTask->ReadyForActivation();
	}
}

void UGA_EnemyAttackBase::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyAttackBase::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_EnemyAttackBase::OnHitCheckEventReceived(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Log, TEXT("몽타주 노티파이 실행"));

	// TODO: 데미지 부여
	if (GameplayCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(GameplayCueTag, FGameplayCueParameters());
	}
}

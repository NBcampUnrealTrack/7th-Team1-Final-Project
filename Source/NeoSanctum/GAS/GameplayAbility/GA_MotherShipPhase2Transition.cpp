// Copyright 2026 One Team. All rights reserved.


#include "GA_MotherShipPhase2Transition.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "NeoSanctum/Character/Enemy/NSBossMotherShip.h"

UGA_MotherShipPhase2Transition::UGA_MotherShipPhase2Transition()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy  = EGameplayAbilityNetSecurityPolicy::ServerOnly;
}

void UGA_MotherShipPhase2Transition::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	ANSBossMotherShip* MotherShip = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo());
	if (!MotherShip)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	MotherShip->BeginPhase2Transition();
	
	// 연출 대기: 몽타주 있으면 재생, 없으면 고정 시간 대기 → 끝나면 OnTransitionSequenceFinished
	if (TransitionMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, TEXT("Phase2TransitionMontage"), TransitionMontage);

		// 완료/중단/취소 어느 쪽이든 마무리(보스가 무적에 갇히지 않게 전부 같은 핸들러로)
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		MontageTask->ReadyForActivation();
	}
	else
	{
		UAbilityTask_WaitDelay* WaitTask =
			UAbilityTask_WaitDelay::WaitDelay(this, TransitionDuration);

		WaitTask->OnFinish.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		WaitTask->ReadyForActivation();
	}
}

void UGA_MotherShipPhase2Transition::OnTransitionSequenceFinished()
{
	if (!IsActive()) return;
	
	ANSBossMotherShip* MotherShip = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo());
	if (!MotherShip) return;
	
	MotherShip->CompletePhase2Transition();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

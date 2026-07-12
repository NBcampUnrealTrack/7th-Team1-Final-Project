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
	
	// 보스 몬스터 캐스팅
	ANSBossMotherShip* MotherShip = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo());
	if (!MotherShip)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// 2페이즈 연출 시작 
	MotherShip->BeginPhase2Transition();
	
	// 몽타주가 존재한다면
	if (TransitionMontage)
	{
		// 등록된 몽타주로 Phase2 몽타주태스크 만듦
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, TEXT("Phase2TransitionMontage"), TransitionMontage);
		
		// 만든 몽타주 태스크에 함수 바인딩 몽타주가 취소/스킵/재생완료 3가지 모든 경우에도 이벤트스킵방지
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		
		// 몽타주 준비 완료 호출
		MontageTask->ReadyForActivation();
	}
	else // 몽타주가 없다면
	{
		// 일정시간동안 대기시키는 대기 테스크 제작
		UAbilityTask_WaitDelay* WaitTask =
			UAbilityTask_WaitDelay::WaitDelay(this, TransitionDuration);
		
		// 대기시간이 전부다 끝나면 이벤트 작동하게 바인딩
		WaitTask->OnFinish.AddDynamic(this, &ThisClass::OnTransitionSequenceFinished);
		WaitTask->ReadyForActivation();
	}
}

// 엔진이 EndAbility를 강제 호출할경우 보스가 무적 및 이동잠금에 갇히는 버그가 발생 할 수도 있음 가능성 매우 낮음
void UGA_MotherShipPhase2Transition::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 보스 몬스터 캐스팅
	if (ANSBossMotherShip* MotherShip = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo()))
	{
		// 숨겼던 2페이즈 태그 주입 함수 호출
		MotherShip->CommitDeferredPhase2Tag();
	}
	
	// 부모 EndAbility호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MotherShipPhase2Transition::OnTransitionSequenceFinished()
{
	if (!IsActive()) return;
	
	ANSBossMotherShip* MotherShip = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo());
	if (!MotherShip) return;
	
	MotherShip->CompletePhase2Transition();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

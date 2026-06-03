// Copyright 2026 One Team. All rights reserved.


#include "GA_ThrowProjectile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_ThrowProjectile::UGA_ThrowProjectile()
{
	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
}

void UGA_ThrowProjectile::ActivateAbility(
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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!AnimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 시작
	ThrowMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("ThrowMontageTask"),
		AnimMontage,
		MontagePlayRate,
		NAME_None,
		false,
		1.0f,
		0.0f
	);

	if (!ThrowMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ThrowMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnThrowMontageCompleted);
	ThrowMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnThrowMontageInterrupted);
	ThrowMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnThrowMontageInterrupted);
	ThrowMontageTask->ReadyForActivation();
	
	if (ActorInfo->IsLocallyControlled())
	{
		// TODO : 던지기 궤적 + 탄착 지점 프리뷰
	}
}

void UGA_ThrowProjectile::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			
			ASC->CurrentMontageJumpToSection(ReleaseSectionName);
		}
	}
}

void UGA_ThrowProjectile::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// TODO : 프리뷰 종료

	// 몽타주 종료
	if (ThrowMontageTask)
	{
		ThrowMontageTask->EndTask();
		ThrowMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ThrowProjectile::OnThrowMontageCompleted()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}

void UGA_ThrowProjectile::OnThrowMontageInterrupted()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		true
	);
}

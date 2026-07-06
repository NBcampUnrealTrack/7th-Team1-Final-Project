// Copyright 2026 One Team. All rights reserved.


#include "GA_EngineerBarrier.h"

UGA_EngineerBarrier::UGA_EngineerBarrier()
{
}

void UGA_EngineerBarrier::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!HasValidBarrierConfig())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float BarrierRadius = 0.0f;
	if (!TryGetBarrierRadius(BarrierRadius))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float BarrierDuration = 0.0f;
	if (!TryGetBarrierDuration(BarrierDuration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RebuildSetByCallerMagnitudes();
	SpawnBarrierActor(ActorInfo, BarrierRadius, BarrierDuration, GetSetByCallerMagnitudes());
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

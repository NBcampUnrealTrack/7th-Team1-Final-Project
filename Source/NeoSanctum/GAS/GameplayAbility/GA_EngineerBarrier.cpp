// Copyright 2026 One Team. All rights reserved.


#include "GA_EngineerBarrier.h"

#include "AbilitySystemComponent.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSBarrierBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"

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
	if (ANSBarrierBase* SpawnedBarrier =
		SpawnBarrierActor(ActorInfo, BarrierRadius, BarrierDuration, GetSetByCallerMagnitudes()))
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Instigator = ActorInfo->AvatarActor.Get();
		CueParameters.Location = SpawnedBarrier->GetActorLocation();
		CueParameters.Normal = SpawnedBarrier->GetActorUpVector();
		CueParameters.RawMagnitude = BarrierRadius;

		ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_Barrier_Deploy, CueParameters);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

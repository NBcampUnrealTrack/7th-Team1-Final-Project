// Copyright 2026 One Team. All rights reserved.


#include "GA_ThrowProjectile.h"

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
		// TODO : 실제 던지기 = Projectile 액터를 스폰
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_ThrowProjectile::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// TODO : 프리뷰 종료

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

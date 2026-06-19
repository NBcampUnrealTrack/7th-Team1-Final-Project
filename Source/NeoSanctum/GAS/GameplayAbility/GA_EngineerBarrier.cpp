// Copyright 2026 One Team. All rights reserved.


#include "GA_EngineerBarrier.h"

#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"

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

	if (!BarrierActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

void UGA_EngineerBarrier::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EngineerBarrier::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo
) const
{
	if (!CooldownGameplayEffectClass)
	{
		return;
	}
	
	float CooldownDuration = 0.0f;
	
	if (!TryGetFinalCooldownDuration(CooldownDuration))
	{
		return;
	}
	
	FGameplayEffectSpecHandle CooldownSpecHandle =
		MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
	
	if (!CooldownSpecHandle.IsValid() || !CooldownSpecHandle.Data.IsValid())
	{
		return;
	}
	
	CooldownSpecHandle.Data->SetSetByCallerMagnitude(
		CooldownEffectTag,
		CooldownDuration
	);
	
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpecHandle);
}

bool UGA_EngineerBarrier::TryGetFinalCooldownDuration(float& OutCooldownDuration) const
{
	float FinalCooldownDuration = 0.0f;
	
	if (!TryGetFinalAbilityStat(
		AbilityTag,
		NSGameplayTags::CombatStat_Cooldown,
		FinalCooldownDuration))
	{
		return false;
	}
	
	constexpr float MinCooldownDuration = 0.1f;
	OutCooldownDuration = FMath::Max(FinalCooldownDuration, MinCooldownDuration);
	
	return true;
}

bool UGA_EngineerBarrier::TryGetBarrierRadius(float& OutBarrierRadius) const
{
	float FinalBarrierRadius = 0.0f;
	
	if (!TryGetFinalAbilityStat(
		AbilityTag,
		NSGameplayTags::CombatStat_Radius,
		FinalBarrierRadius))
	{
		return false;
	}
	
	OutBarrierRadius = FMath::Max(FinalBarrierRadius, MinimumBarrierRadius);
	
	return true;
}

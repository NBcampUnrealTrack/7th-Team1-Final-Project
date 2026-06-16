// Copyright 2026 One Team. All rights reserved.


#include "GA_Reload.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_Reload::UGA_Reload()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	SetAssetTags(AssetTags);

	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	ReloadSpeedStatTag = NSGameplayTags::CombatStat_ReloadSpeed;

	ActivationBlockedTags.AddTag(NSGameplayTags::State_Reloading);
}

void UGA_Reload::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "Reload activation failed. ActorInfo or AvatarActor is invalid.");
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float ReloadDuration = 0.0f;

	if (!TryGetFinalReloadDuration(ReloadDuration))
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Warning,
			"ReloadSpeed CombatStat lookup failed. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", GetReloadCombatStatAbilityTag().ToString()),
			("StatTag", ReloadSpeedStatTag.ToString())
		);

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Warning, "Reload CommitAbility failed.");
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC)
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Warning, "Reload activation failed. ASC is invalid.");
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 재장전 중복 실행 방지
	ASC->AddLooseGameplayTag(NSGameplayTags::State_Reloading);
	AddDeactivateHandIKTag();

	PlayReloadMontage(ReloadDuration);

	UWorld* World = GetWorld();

	if (!World)
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Warning, "Reload activation failed. World is invalid.");
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ReloadSpeed는 높을수록 빠른 값으로 보고 시간으로 변환
	World->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&ThisClass::FinishReload,
		ReloadDuration,
		false
	);
}

void UGA_Reload::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 재장전 상태태그 제거
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Reloading);
	}

	RemoveDeactivateHandIKTag();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_Reload::CheckCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "Reload CheckCost failed. ActorInfo or ASC is invalid.");
		return false;
	}

	const UNSPlayerAttributeSet* AttributeSet =
		ActorInfo->AbilitySystemComponent->GetSet<UNSPlayerAttributeSet>();

	if (!AttributeSet)
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "Reload CheckCost failed. NSPlayerAttributeSet is missing.");
		return false;
	}

	// 탄약이 이미 가득 차 있으면 재장전하지 않음
	if (AttributeSet->GetAmmo() >= AttributeSet->GetMaxAmmo())
	{
		NS_OBJ_LOG(LogNSGAS, Warning,
			"Reload CheckCost failed. Ammo is already full. Ammo={Ammo}, MaxAmmo={MaxAmmo}",
			("Ammo", AttributeSet->GetAmmo()),
			("MaxAmmo", AttributeSet->GetMaxAmmo())
		);

		return false;
	}

	return AttributeSet->GetAmmo() < AttributeSet->GetMaxAmmo();
}

void UGA_Reload::FinishReload()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (ActorInfo && ActorInfo->IsNetAuthority() && ASC)
	{
		if (const UNSPlayerAttributeSet* AttributeSet = ASC->GetSet<UNSPlayerAttributeSet>())
		{
			// 서버에서 최종 탄약 수 복구
			const float MaxAmmo = FMath::Max(AttributeSet->GetMaxAmmo(), 0.0f);
			ASC->ApplyModToAttribute(UNSPlayerAttributeSet::GetAmmoAttribute(), EGameplayModOp::Override, MaxAmmo);
		}
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}

void UGA_Reload::PlayReloadMontage(float ReloadDuration)
{
	if (!ReloadMontage || ReloadDuration <= 0.0f)
	{
		return;
	}

	const float MontageLength = ReloadMontage->GetPlayLength();

	if (MontageLength <= 0.0f)
	{
		return;
	}

	const float ClampedMaxPlayRate = FMath::Max(MaxMontagePlayRate, 0.01f);
	const float MontagePlayRate = FMath::Clamp(MontageLength / ReloadDuration, 0.01f, ClampedMaxPlayRate);

	// 실제 재장전 완료는 타이머가 담당하고 몽타주는 시간에 맞춰 재생
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			ReloadMontage,
			MontagePlayRate,
			NAME_None,
			true
		);

	if (!MontageTask)
	{
		return;
	}

	MontageTask->ReadyForActivation();
}

void UGA_Reload::AddDeactivateHandIKTag()
{
	if (bDeactivateHandIKTagAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(NSGameplayTags::State_Deactivate_HandIK);
		bDeactivateHandIKTagAdded = true;
	}
}

void UGA_Reload::RemoveDeactivateHandIKTag()
{
	if (!bDeactivateHandIKTagAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Deactivate_HandIK);
	}

	bDeactivateHandIKTagAdded = false;
}

bool UGA_Reload::TryGetFinalReloadDuration(float& OutReloadDuration) const
{
	float FinalReloadSpeed = 0.0f;

	if (!TryGetFinalAbilityStat(
		GetReloadCombatStatAbilityTag(),
		ReloadSpeedStatTag,
		FinalReloadSpeed))
	{
		return false;
	}

	constexpr float MinReloadSpeed = 0.01f;
	FinalReloadSpeed = FMath::Max(FinalReloadSpeed, MinReloadSpeed);
	OutReloadDuration = FMath::Max(1.0f / FinalReloadSpeed, MinReloadDuration);

	return true;
}

FGameplayTag UGA_Reload::GetReloadCombatStatAbilityTag() const
{
	return CombatStatAbilityTag;
}

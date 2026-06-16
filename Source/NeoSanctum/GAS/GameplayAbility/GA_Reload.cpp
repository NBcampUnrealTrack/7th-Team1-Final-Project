// Copyright 2026 One Team. All rights reserved.


#include "GA_Reload.h"

#include "AbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_Reload::UGA_Reload()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Common_Reload);
	SetAssetTags(AssetTags);

	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	CombatStatAbilityTag = NSGameplayTags::Ability_Common_Reload;
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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float ReloadDuration = 0.0f;

	if (!TryGetFinalReloadDuration(ReloadDuration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	// 재장전 중복 실행 방지
	ASC->AddLooseGameplayTag(NSGameplayTags::State_Reloading);

	UWorld* World = GetWorld();

	if (!World)
	{
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
		return false;
	}

	const UNSPlayerAttributeSet* AttributeSet =
		ActorInfo->AbilitySystemComponent->GetSet<UNSPlayerAttributeSet>();

	if (!AttributeSet)
	{
		return false;
	}

	// 탄약이 이미 가득 차 있으면 재장전하지 않음
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
	if (CombatStatAbilityTag.IsValid())
	{
		return CombatStatAbilityTag;
	}

	return NSGameplayTags::Ability_Common_Reload;
}

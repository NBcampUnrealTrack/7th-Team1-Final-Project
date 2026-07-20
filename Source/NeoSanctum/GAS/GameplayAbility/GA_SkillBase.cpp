// Copyright 2026 One Team. All rights reserved.


#include "GA_SkillBase.h"

#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_SkillBase::UGA_SkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

bool UGA_SkillBase::CheckCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bHasCost = Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	
	if (bHasCost || !bRequestReloadOnEmptyAmmo)
	{
		return bHasCost;
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
	
	if (AttributeSet->GetAmmo() > 0.0f || AttributeSet->GetMaxAmmo() <= 0.0)
	{
		return false;
	}
	
	if (OptionalRelevantTags)
	{
		OptionalRelevantTags->AddTag(NSGameplayTags::Ability_ActivateFail_OutOfAmmo);
	}
	
	return false;
}

bool UGA_SkillBase::CommitAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	FGameplayTagContainer* OptionalRelevantTags)
{
	const bool bCommitted = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);
	if (bCommitted)
	{
		// Cost 소모가 확정된 뒤에 충전 회복을 시작
		StartRechargeIfNeeded();
	}

	return bCommitted;
}

bool UGA_SkillBase::TryGetBaseAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const UNSAbilitySystemComponent* NSASC =
		Cast<UNSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	
	if (!NSASC)
	{
		return false;
	}
	
	return NSASC->TryGetBaseAbilityStat(AbilityTag, StatTag, OutValue);
}

bool UGA_SkillBase::TryGetFinalAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const UNSAbilitySystemComponent* NSASC = 
		Cast<UNSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	
	if (!NSASC)
	{
		return false;
	}
	
	return NSASC->TryGetFinalAbilityStat(AbilityTag, StatTag, OutValue);
}

float UGA_SkillBase::GetFinalAbilityStatOrDefault(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float DefaultValue) const
{
	float Value = DefaultValue;
	
	TryGetFinalAbilityStat(AbilityTag, StatTag, Value);
	
	return Value;
}

float UGA_SkillBase::GetBaseAbilityStatOrDefault(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float DefaultValue) const
{
	float Value = DefaultValue;
	
	TryGetBaseAbilityStat(AbilityTag, StatTag, Value);
	
	return Value;
}

bool UGA_SkillBase::TryGetFinalSkillDamage(const FGameplayTag& AbilityTag, float& OutDamage) const
{
	float CoefficientPercent = 0.0f;

	if (!TryGetFinalAbilityStat(AbilityTag, NSGameplayTags::CombatStat_DamageCoefficient, CoefficientPercent))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		return false;
	}

	const float PlayerBaseDamage = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetBaseDamageAttribute());

	// 계수는 % 단위 (30 = 30%)로 입력받으므로 0.01을 곱해 배율로 변환
	OutDamage = FMath::Max(PlayerBaseDamage * CoefficientPercent * 0.01f, 0.0f);

	return true;
}

bool UGA_SkillBase::TryCalculateDamageFalloffMultiplier(
	const FGameplayTag& AbilityTag,
	float HitDistance,
	float& OutDamageMultiplier) const
{
	OutDamageMultiplier = 1.0f;

	float FalloffStartDistance = 0.0f;
	float FalloffEndDistance = 0.0f;
	float MinimumDamageMultiplier = 0.0f;

	if (!TryGetFinalAbilityStat(
	AbilityTag,
	NSGameplayTags::CombatStat_DamageFalloffStartDistance,
	FalloffStartDistance))
	{
		return false;
	}

	if (!TryGetFinalAbilityStat(
		AbilityTag,
		NSGameplayTags::CombatStat_DamageFalloffEndDistance,
		FalloffEndDistance))
	{
		return false;
	}

	if (!TryGetFinalAbilityStat(
		AbilityTag,
		NSGameplayTags::CombatStat_DamageFalloffMinimumMultiplier,
		MinimumDamageMultiplier))
	{
		return false;
	}

	// 감쇠 구간과 최소 배율이 올바르지 않으면 잘못된 데미지를 적용하지 않음.
	if (FalloffStartDistance < 0.0f || FalloffEndDistance <= FalloffStartDistance ||
		MinimumDamageMultiplier < 0.0f || MinimumDamageMultiplier > 1.0f)
	{
		return false;
	}

	const float ClampedHitDistance = FMath::Max(HitDistance, 0.0f);

	if (ClampedHitDistance <= FalloffStartDistance)
	{
		OutDamageMultiplier = 1.0f;
		return true;
	}

	if (ClampedHitDistance >= FalloffEndDistance)
	{
		// 종료 거리 이후에는 최소 배율을 그대로 유지.
		OutDamageMultiplier = MinimumDamageMultiplier;
		return true;
	}

	const float FalloffAlpha = FMath::GetRangePct(
		FalloffStartDistance, FalloffEndDistance, ClampedHitDistance);

	OutDamageMultiplier = FMath::Lerp(1.0f, MinimumDamageMultiplier, FalloffAlpha);

	return true;
}

bool UGA_SkillBase::TryReportAbilityNoise(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& LoudnessStatTag,
	const FVector& NoiseLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APawn* NoiseInstigator = Cast<APawn>(AvatarActor);
	
	if (!IsValid(NoiseInstigator) || !NoiseInstigator->HasAuthority())
	{
		return false;
	}
	
	float Loudness = 0.0f;
	
	if (!TryGetFinalAbilityStat(AbilityTag, LoudnessStatTag, Loudness))
	{
		return false;
	}
	
	Loudness = FMath::Max(Loudness, 0.0f);
	
	if (Loudness <= 0.0f)
	{
		return false;
	}
	
	NoiseInstigator->MakeNoise(Loudness, NoiseInstigator, NoiseLocation);
	
	return true;
}

float UGA_SkillBase::GetCooldownStatOrDefault() const
{
	if (!SkillAbilityTag.IsValid())
	{
		return DefaultCooldown;
	}
	
	// 최종 스탯 기준으로 반환
	return GetFinalAbilityStatOrDefault(
		SkillAbilityTag,
		NSGameplayTags::CombatStat_Cooldown,
		DefaultCooldown
	);
}

void UGA_SkillBase::StartRechargeIfNeeded()
{
	if (!SkillSlotTag.IsValid())
	{
		return;
	}

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	UNSAbilitySystemComponent* NSASC = GetNSAbilitySystemComponent();
	if (!NSASC)
	{
		return;
	}

	const float Cooldown = GetCooldownStatOrDefault();
	if (Cooldown <= 0.0f)
	{
		return;
	}
	
	// 실제로 해당 스킬 슬롯의 Recharge를 시작
	NSASC->StartSkillRecharge(SkillSlotTag, Cooldown);
}

UNSAbilitySystemComponent* UGA_SkillBase::GetNSAbilitySystemComponent() const
{
	return Cast<UNSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

ANSPlayerState* UGA_SkillBase::GetNSPlayerState() const
{
	return Cast<ANSPlayerState>(GetOwningActorFromActorInfo());
}

void UGA_SkillBase::ReportShotsFired(int32 Count)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	if (ANSPlayerState* NSPS = GetNSPlayerState())
	{
		NSPS->AddShotsFired(Count);
	}
}

void UGA_SkillBase::ReportShotsHit(int32 Count)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	if (ANSPlayerState* NSPS = GetNSPlayerState())
	{
		NSPS->AddShotsHit(Count);
	}
}

FString UGA_SkillBase::GetCurrentPredictionKeyStatus()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return TEXT("NSAbilitySystemComponent 없음");
	}
	
	return ASC->ScopedPredictionKey.ToString() + TEXT("예측키가 아직 열려 있나: ")
		+ (ASC->ScopedPredictionKey.IsValidForMorePrediction() ? TEXT("열려있음") : TEXT("닫힘"));
}

bool UGA_SkillBase::IsPredictionKeyValidForMorePrediction() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}
	
	return ASC->ScopedPredictionKey.IsValidForMorePrediction();
}

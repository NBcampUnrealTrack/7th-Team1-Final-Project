// Copyright 2026 One Team. All rights reserved.


#include "GEC_DamageExecution.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"

namespace
{
	struct FNSDamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(BaseDamage);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamage);
		
		FNSDamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UNSBaseAttributeSet, BaseDamage, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UNSBaseAttributeSet, Defense, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UNSPlayerAttributeSet, CritChance, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UNSPlayerAttributeSet, CritDamage, Source, false);
		}
	};
	
	const FNSDamageStatics& GetDamageStatics()
	{
		static FNSDamageStatics DamageStatics;
		return DamageStatics;
	}
}

UGEC_DamageExecution::UGEC_DamageExecution()
{
	RelevantAttributesToCapture.Add(GetDamageStatics().BaseDamageDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().CritChanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().CritDamageDef);
}

void UGEC_DamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	float SourceBaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetDamageStatics().BaseDamageDef,
		EvaluateParameters,
		SourceBaseDamage
	);
	
	float TargetDefense = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetDamageStatics().DefenseDef,
		EvaluateParameters,
		TargetDefense
	);

	// Source가 NSPlayerAttributeSet이 없는 대상(몬스터/터렛 등)이면 캡처가 실패하므로,
	// 크리티컬이 발생하지 않는 기본값(확률 0%, 배율 100%)을 미리 채워둠.
	float SourceCritChance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetDamageStatics().CritChanceDef,
		EvaluateParameters,
		SourceCritChance
	);

	float SourceCritDamage = 100.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetDamageStatics().CritDamageDef,
		EvaluateParameters,
		SourceCritDamage
	);

	SourceBaseDamage = FMath::Max(SourceBaseDamage, 0.0f);
	TargetDefense = FMath::Max(TargetDefense, 0.0f);
	
	const FGameplayTag DamageBaseTag = NSGameplayTags::Effect_Damage_Base.GetTag();
	
	const float AppliedBaseDamage = Spec.GetSetByCallerMagnitude(
		DamageBaseTag,
		false,
		SourceBaseDamage
	);

	// Turret처럼 Source ASC에 CritChance/CritDamage Attribute가 없어 캡처가 실패한 경우,
	// 발사 시점에 소환자의 실제 값을 SetByCaller로 전달받아 덮어씀. 일반 플레이어 공격은 이 태그를 설정하지 않으므로 캡처값이 그대로 사용됨.
	SourceCritChance = Spec.GetSetByCallerMagnitude(
		NSGameplayTags::Effect_Damage_CritChanceOverride.GetTag(),
		false,
		SourceCritChance
	);

	SourceCritDamage = Spec.GetSetByCallerMagnitude(
		NSGameplayTags::Effect_Damage_CritDamageOverride.GetTag(),
		false,
		SourceCritDamage
	);

	// 방어력 감소 배율: y = k / (k + Defense)
	float DefenseMitigationConstant = 100.0f;
	if (const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
	{
		if (const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(TargetASC))
		{
			DefenseMitigationConstant = DataSubsystem->GetDefenseMitigationConstant();
		}
	}

	const float MitigationMultiplier = DefenseMitigationConstant / (DefenseMitigationConstant + TargetDefense);

	// 크리티컬 판정 및 배율. 방어력 배율과 곱셈 순서가 상관 없음.
	const bool bIsCritical = FMath::FRandRange(0.0f, 100.0f) < SourceCritChance;
	const float CritMultiplier = bIsCritical ? (SourceCritDamage / 100.0f) : 1.0f;

	const float FinalDamage = FMath::Max(AppliedBaseDamage * MitigationMultiplier * CritMultiplier, 0.0f);

	if (FinalDamage <= 0.0f)
	{
		return;
	}

	const ENSHitFeedbackQuality DamageHitQuality = bIsCritical
		? ENSHitFeedbackQuality::Critical
		: ENSHitFeedbackQuality::Normal;

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UNSBaseAttributeSet::GetDamageHitQualityAttribute(),
			EGameplayModOp::Override,
			static_cast<float>(static_cast<uint8>(DamageHitQuality))
		)
	);
	
	// Shield 흡수와 Health 차감은 AttributeSet에서 처리하도록 Damage Meta Attribute에 출력
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UNSBaseAttributeSet::GetDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage
		)
	);
}

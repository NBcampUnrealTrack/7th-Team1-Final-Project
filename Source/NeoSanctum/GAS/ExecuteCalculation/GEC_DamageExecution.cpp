// Copyright 2026 One Team. All rights reserved.


#include "GEC_DamageExecution.h"

#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"

namespace
{
	struct FNSDamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(BaseDamage);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
		
		FNSDamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UNSBaseAttributeSet, BaseDamage, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UNSBaseAttributeSet, Defense, Target, false);
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
	
	SourceBaseDamage = FMath::Max(SourceBaseDamage, 0.0f);
	TargetDefense = FMath::Max(TargetDefense, 0.0f);
	
	const float FinalDamage = FMath::Max(SourceBaseDamage - TargetDefense, 0.0f);
	
	if (FinalDamage <= 0.0f)
	{
		return;
	}
	
	// Shield 흡수와 Health 차감은 AttributeSet에서 처리하도록 Damage Meta Attribute에 출력
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UNSBaseAttributeSet::GetDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage
		)
	);
}

// Copyright 2026 One Team. All rights reserved.


#include "NSTurretAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UNSTurretAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSTurretAttributeSet, DetectionRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSTurretAttributeSet, FireRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSTurretAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSTurretAttributeSet, Accuracy, COND_None, REPNOTIFY_Always);
}

void UNSTurretAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetDetectionRangeAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetFireRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetAttackRangeAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetAccuracyAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNSTurretAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDetectionRangeAttribute())
	{
		SetDetectionRange(FMath::Max(GetDetectionRange(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetFireRateAttribute())
	{
		SetFireRate(FMath::Max(GetFireRate(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackRangeAttribute())
	{
		SetAttackRange(FMath::Max(GetAttackRange(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAccuracyAttribute())
	{
		SetAccuracy(FMath::Max(GetAccuracy(), 0.0f));
	}
}

void UNSTurretAttributeSet::OnRep_DetectionRange(const FGameplayAttributeData& OldDetectionRange)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSTurretAttributeSet, DetectionRange, OldDetectionRange);
}

void UNSTurretAttributeSet::OnRep_FireRate(const FGameplayAttributeData& OldFireRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSTurretAttributeSet, FireRate, OldFireRate);
}

void UNSTurretAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAttackRange)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSTurretAttributeSet, AttackRange, OldAttackRange);
}

void UNSTurretAttributeSet::OnRep_Accuracy(const FGameplayAttributeData& OldAccuracy)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSTurretAttributeSet, Accuracy, OldAccuracy);
}

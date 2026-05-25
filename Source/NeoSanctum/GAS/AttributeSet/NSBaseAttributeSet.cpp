// Copyright 2026 One Team. All rights reserved.


#include "NSBaseAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UNSBaseAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, BaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, DashCost, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSBaseAttributeSet, MaxDashCost, COND_None, REPNOTIFY_Always);
}

void UNSBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetBaseDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDefenseAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDashCostAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDashCost());
	}
	else if (Attribute == GetMaxDashCostAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNSBaseAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float DamageAmount = GetDamage();
		SetDamage(0.0f);
		
		if (DamageAmount <= 0.0f)
		{
			return;
		}
		
		// Health 적용 전 대상별 방어 처리(자식 함수에서 재정의)
		DamageAmount = HandlePreHealthDamage(DamageAmount, Data);
		
		if (DamageAmount <= 0.0f)
		{
			return;
		}
		
		const float NewHealth = FMath::Clamp(GetHealth() - DamageAmount, 0.0f, GetMaxHealth());
		
		SetHealth(NewHealth);
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Max(GetMoveSpeed(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetDashCostAttribute())
	{
		SetDashCost(FMath::Clamp(GetDashCost(), 0.0f, GetMaxDashCost()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxDashCostAttribute())
	{
		SetMaxDashCost(FMath::Max(GetMaxDashCost(), 0.0f));
		SetDashCost(FMath::Clamp(GetDashCost(), 0.0f, GetMaxDashCost()));
	}
}

float UNSBaseAttributeSet::HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData&)
{
	return DamageAmount;
}

void UNSBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, Health, OldHealth);
}

void UNSBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, MaxHealth, OldMaxHealth);
}

void UNSBaseAttributeSet::OnRep_BaseDamage(const FGameplayAttributeData& OldBaseDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, BaseDamage, OldBaseDamage);
}

void UNSBaseAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, Defense, OldDefense);
}

void UNSBaseAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, MoveSpeed, OldMoveSpeed);
}

void UNSBaseAttributeSet::OnRep_DashCost(const FGameplayAttributeData& OldDashCost)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, DashCost, OldDashCost);
}

void UNSBaseAttributeSet::OnRep_MaxDashCost(const FGameplayAttributeData& OldMaxDashCost)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSBaseAttributeSet, MaxDashCost, OldMaxDashCost);
}

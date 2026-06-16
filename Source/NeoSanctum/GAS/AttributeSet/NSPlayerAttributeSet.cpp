// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UNSPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, DashCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxDashCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, DashRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Ammo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxAmmo, COND_None, REPNOTIFY_Always);
}

void UNSPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDashCountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDashCount());
	}
	else if (Attribute == GetMaxDashCountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDashRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetAmmoAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxAmmo());
	}
	else if (Attribute == GetMaxAmmoAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNSPlayerAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxShieldAttribute())
	{
		SetMaxShield(FMath::Max(GetMaxShield(), 0.0f));
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));
	}
	else if (Data.EvaluatedData.Attribute == GetDashCountAttribute())
	{
		SetDashCount(FMath::Clamp(GetDashCount(), 0.0f, GetMaxDashCount()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxDashCountAttribute())
	{
		SetMaxDashCount(FMath::Max(GetMaxDashCount(), 0.0f));
		SetDashCount(FMath::Clamp(GetDashCount(), 0.0f, GetMaxDashCount()));
	}
	else if (Data.EvaluatedData.Attribute == GetDashRegenRateAttribute())
	{
		SetDashRegenRate(FMath::Max(GetDashRegenRate(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAmmoAttribute())
	{
		SetAmmo(FMath::Clamp(GetAmmo(), 0.0f, GetMaxAmmo()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxAmmoAttribute())
	{
		SetMaxAmmo(FMath::Max(GetMaxAmmo(), 0.0f));
		SetAmmo(FMath::Clamp(GetAmmo(), 0.0f, GetMaxAmmo()));
	}
}

float UNSPlayerAttributeSet::HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData&)
{
	const float CurrentShield = GetShield();
	
	if (CurrentShield <= 0.0f)
	{
		return DamageAmount;
	}
	
	const float AbsorbedDamage = FMath::Min(CurrentShield, DamageAmount);
	SetShield(CurrentShield - AbsorbedDamage);
	
	return DamageAmount - AbsorbedDamage;
}

void UNSPlayerAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Shield, OldShield);
}

void UNSPlayerAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxShield, OldMaxShield);
}

void UNSPlayerAttributeSet::OnRep_DashCount(const FGameplayAttributeData& OldDashCount)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, DashCount, OldDashCount);
}

void UNSPlayerAttributeSet::OnRep_MaxDashCount(const FGameplayAttributeData& OldMaxDashCount)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxDashCount, OldMaxDashCount);
}

void UNSPlayerAttributeSet::OnRep_DashRegenRate(const FGameplayAttributeData& OldDashRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, DashRegenRate, OldDashRegenRate);
}

void UNSPlayerAttributeSet::OnRep_Ammo(const FGameplayAttributeData& OldAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Ammo, OldAmmo);
}

void UNSPlayerAttributeSet::OnRep_MaxAmmo(const FGameplayAttributeData& OldMaxAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxAmmo, OldMaxAmmo);
}

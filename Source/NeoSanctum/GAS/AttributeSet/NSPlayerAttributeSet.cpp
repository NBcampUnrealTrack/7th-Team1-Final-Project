// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UNSPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
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
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		
		if (!bOutOfHealth && GetHealth() <= 0.0f)
		{
			bOutOfHealth = true;
			OnOutOfHealth.Broadcast();
		}
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

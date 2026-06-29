// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "Net/UnrealNetwork.h"

void UNSPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, ShieldRechargeRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, ShieldRechargeCooldown, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, DashCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxDashCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, DashRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Ammo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxSkill1Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Skill1Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxSkill2Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Skill2Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxSkill3Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Skill3Count, COND_None, REPNOTIFY_Always);
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
	else if (Attribute == GetShieldRechargeRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetShieldRechargeCooldownAttribute())
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
	else if (Attribute == GetMaxSkill1CountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetSkill1CountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSkill1Count());
	}
	else if (Attribute == GetMaxSkill2CountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetSkill2CountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSkill2Count());
	}
	else if (Attribute == GetMaxSkill3CountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetSkill3CountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSkill3Count());
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
	else if (Data.EvaluatedData.Attribute == GetShieldRechargeRateAttribute())
	{
		SetShieldRechargeRate(FMath::Max(GetShieldRechargeRate(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetShieldRechargeCooldownAttribute())
	{
		SetShieldRechargeCooldown(FMath::Max(GetShieldRechargeCooldown(), 0.0f));
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
	else if (Data.EvaluatedData.Attribute == GetMaxSkill1CountAttribute())
	{
		SetMaxSkill1Count(FMath::Max(GetMaxSkill1Count(), 0.0f));
		SetSkill1Count(FMath::Clamp(GetSkill1Count(), 0.0f, GetMaxSkill1Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetSkill1CountAttribute())
	{
		SetSkill1Count(FMath::Clamp(GetSkill1Count(), 0.0f, GetMaxSkill1Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxSkill2CountAttribute())
	{
		SetMaxSkill2Count(FMath::Max(GetMaxSkill2Count(), 0.0f));
		SetSkill2Count(FMath::Clamp(GetSkill2Count(), 0.0f, GetMaxSkill2Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetSkill2CountAttribute())
	{
		SetSkill2Count(FMath::Clamp(GetSkill2Count(), 0.0f, GetMaxSkill2Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxSkill3CountAttribute())
	{
		SetMaxSkill3Count(FMath::Max(GetMaxSkill3Count(), 0.0f));
		SetSkill3Count(FMath::Clamp(GetSkill3Count(), 0.0f, GetMaxSkill3Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetSkill3CountAttribute())
	{
		SetSkill3Count(FMath::Clamp(GetSkill3Count(), 0.0f, GetMaxSkill3Count()));
	}
}

float UNSPlayerAttributeSet::HandlePreHealthDamage(
	float DamageAmount,
	const FGameplayEffectModCallbackData& Data)
{
	const float CurrentShield = GetShield();
	
	if (CurrentShield <= 0.0f)
	{
		return DamageAmount;
	}
	
	const float AbsorbedDamage = FMath::Min(CurrentShield, DamageAmount);
	const float NewShield = CurrentShield - AbsorbedDamage;
	SetShield(NewShield);

	if (NewShield <= 0.0f)
	{
		if (ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(Data.Target.GetAvatarActor()))
		{
			PlayerCharacter->ApplyReactiveGameplayEffect(NSGameplayTags::Event_Common_Shield_Broken);
		}
	}
	
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

void UNSPlayerAttributeSet::OnRep_ShieldRechargeRate(const FGameplayAttributeData& OldShieldRechargeRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, ShieldRechargeRate, OldShieldRechargeRate);
}

void UNSPlayerAttributeSet::OnRep_ShieldRechargeCooldown(const FGameplayAttributeData& OldShieldRechargeCooldown)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, ShieldRechargeCooldown, OldShieldRechargeCooldown);
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
void UNSPlayerAttributeSet::OnRep_MaxSkill1Count(const FGameplayAttributeData& OldMaxSkill1Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxSkill1Count, OldMaxSkill1Count);
}

void UNSPlayerAttributeSet::OnRep_Skill1Count(const FGameplayAttributeData& OldSkill1Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill1Count, OldSkill1Count);
}

void UNSPlayerAttributeSet::OnRep_MaxSkill2Count(const FGameplayAttributeData& OldMaxSkill2Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill2Count, OldMaxSkill2Count);
}

void UNSPlayerAttributeSet::OnRep_Skill2Count(const FGameplayAttributeData& OldSkill2Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill2Count, OldSkill2Count);
}

void UNSPlayerAttributeSet::OnRep_MaxSkill3Count(const FGameplayAttributeData& OldMaxSkill3Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill3Count, OldMaxSkill3Count);
}

void UNSPlayerAttributeSet::OnRep_Skill3Count(const FGameplayAttributeData& OldSkill3Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill2Count, OldSkill3Count);
}

// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UNSCompanionAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& OldAttackDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSCompanionAttributeSet, AttackDamage, OldAttackDamage);
}

void UNSCompanionAttributeSet::OnRep_FireRate(const FGameplayAttributeData& OldFireRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSCompanionAttributeSet, FireRate, OldFireRate);
}

void UNSCompanionAttributeSet::OnRep_ProjectileSpeed(const FGameplayAttributeData& OldProjectileSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSCompanionAttributeSet, ProjectileSpeed, OldProjectileSpeed);
}

void UNSCompanionAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAttackRange)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSCompanionAttributeSet, AttackRange, OldAttackRange);
}

void UNSCompanionAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSCompanionAttributeSet, AttackDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSCompanionAttributeSet, FireRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSCompanionAttributeSet, ProjectileSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSCompanionAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
}

void UNSCompanionAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetFireRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.01f);
	}
	else if (Attribute == GetAttackDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetProjectileSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetAttackRangeAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionAttributeSet.h"

void UNSCompanionAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& OldAttackDamage)
{
}

void UNSCompanionAttributeSet::OnRep_FireRate(const FGameplayAttributeData& OldFireRate)
{
}

void UNSCompanionAttributeSet::OnRep_ProjectileSpeed(const FGameplayAttributeData& OldProjectileSpeed)
{
}

void UNSCompanionAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAttackRange)
{
}

void UNSCompanionAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UNSCompanionAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

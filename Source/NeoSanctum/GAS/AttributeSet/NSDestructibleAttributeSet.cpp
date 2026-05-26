// Copyright 2026 One Team. All rights reserved.


#include "NSDestructibleAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UNSDestructibleAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSDestructibleAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSDestructibleAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UNSDestructibleAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float DmgDone = GetDamage();
		SetDamage(0.f);
		SetHealth(FMath::Max(GetHealth() - DmgDone, 0.f));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}

void UNSDestructibleAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSDestructibleAttributeSet, Health, OldHealth);
}

void UNSDestructibleAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSDestructibleAttributeSet, MaxHealth, OldMaxHealth);
}

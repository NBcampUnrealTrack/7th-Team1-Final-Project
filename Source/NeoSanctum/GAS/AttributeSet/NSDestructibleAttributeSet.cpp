// Copyright 2026 One Team. All rights reserved.


#include "NSDestructibleAttributeSet.h"
#include "GameplayEffectExtension.h"

void UNSDestructibleAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// HitResult/Causer/Instigator 모두 포함
		LastDamageContext = Data.EffectSpec.GetContext();
	}
}

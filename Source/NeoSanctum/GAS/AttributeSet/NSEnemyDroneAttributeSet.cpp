// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDroneAttributeSet.h"

void UNSEnemyDroneAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	
	Super::PostGameplayEffectExecute(Data);
}

void UNSEnemyDroneAttributeSet::ReportDamageSenseEvent(const FGameplayEffectModCallbackData& Data) const
{
}

AActor* UNSEnemyDroneAttributeSet::ResolvePerceivedInstigator(AActor* InstigatorActor) const
{
	return nullptr;
}

void UNSEnemyDroneAttributeSet::HandleDeathAfterEffect(ANSEnemyDroneAI* EnemyDrone) const
{
}

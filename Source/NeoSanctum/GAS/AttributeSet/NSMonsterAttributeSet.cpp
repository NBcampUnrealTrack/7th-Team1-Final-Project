// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

void UNSMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute() || Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (GetHealth() <= 0.0f)
		{
			if (ANSEnemyCharacterBase* EnemyCharacter = Cast<ANSEnemyCharacterBase>(Data.Target.GetAvatarActor()))
			{
				EnemyCharacter->Die();
			}
		}
	}
}

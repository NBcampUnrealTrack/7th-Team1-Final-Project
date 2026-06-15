// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "Perception/AISense_Damage.h"

void UNSMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// 부모 AttributeSet에 의해 데미지 처리되기 전에 어그로 감지
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float RawDamage = GetDamage();
		AActor* DamagedActor = Data.Target.GetAvatarActor();
		AActor* InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();

		if (RawDamage > 0.0f && DamagedActor && InstigatorActor)
		{
			AActor* PerceivedActor = InstigatorActor;
			
			if (ANSBaseCompanionAI* AttackingDrone = Cast<ANSBaseCompanionAI>(InstigatorActor))
			{
				if (AActor* OwnerPlayer = AttackingDrone->GetOwnerPlayer())
				{
					PerceivedActor = OwnerPlayer;
				}
			}
			
			APawn* AttackerPawn = Cast<APawn>(PerceivedActor);
			if (!AttackerPawn)
			{
				if (AController* Controller = Cast<AController>(PerceivedActor))
				{
					AttackerPawn = Controller->GetPawn();
				}
			}

			if (AttackerPawn)
			{
				UAISense_Damage::ReportDamageEvent(
					DamagedActor->GetWorld(), 
					DamagedActor, 
					AttackerPawn,
					RawDamage, 
					AttackerPawn->GetActorLocation(), 
					DamagedActor->GetActorLocation()
				);
			}
		}
	}
	
	Super::PostGameplayEffectExecute(Data);

	// 사망 시 처리
	if (GetHealth() <= 0.0f)
	{
		if (ANSEnemyCharacterBase* EnemyCharacter = Cast<ANSEnemyCharacterBase>(Data.Target.GetAvatarActor()))
		{
			EnemyCharacter->Die();
		}
	}
}

// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Damage.h"

void UNSMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, HitGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, MaxHitGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, HitGaugeGainPerHit, COND_None, REPNOTIFY_Always);
}

void UNSMonsterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHitGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, FMath::Max(GetMaxHitGauge(), 1.0f));
	}
	else if (Attribute == GetMaxHitGaugeAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetHitGaugeGainPerHitAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNSMonsterAttributeSet::ResetHitGauge()
{
	SetHitGauge(0.0f);
}

void UNSMonsterAttributeSet::AccumulateHitGauge(ANSEnemyCharacterBase* EnemyCharacter)
{
}

void UNSMonsterAttributeSet::OnRep_HitGauge(const FGameplayAttributeData& OldHitGauge)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, HitGauge, OldHitGauge);
}

void UNSMonsterAttributeSet::OnRep_MaxHitGauge(const FGameplayAttributeData& OldMaxHitGauge)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, MaxHitGauge, OldMaxHitGauge);
}

void UNSMonsterAttributeSet::OnRep_HitGaugeGainPerHit(const FGameplayAttributeData& OldHitGaugeGainPerHit)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, HitGaugeGainPerHit, OldHitGaugeGainPerHit);
}

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

			if (PerceivedActor)
			{
				UAISense_Damage::ReportDamageEvent(
					DamagedActor->GetWorld(),
					DamagedActor,
					PerceivedActor,
					RawDamage,
					PerceivedActor->GetActorLocation(),
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

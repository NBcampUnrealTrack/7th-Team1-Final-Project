// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
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
	if (!EnemyCharacter ||
		!EnemyCharacter->HasAuthority() ||
		EnemyCharacter->IsDead() ||
		EnemyCharacter->IsInPool() ||
		EnemyCharacter->IsHitReacting())
	{
		return;
	}

	const float GaugeGain = FMath::Max(GetHitGaugeGainPerHit(), 0.0f);

	if (GaugeGain <= 0.0f)
	{
		return;
	}

	const float GaugeMaximum = FMath::Max(GetMaxHitGauge(), 1.0f);
	const float NewGauge = FMath::Clamp(GetHitGauge() + GaugeGain, 0.0f, GaugeMaximum);

	SetHitGauge(NewGauge);

	if (NewGauge >= GaugeMaximum)
	{
		// 경직 처리가 현재 최대 게이지를 확인할 수 있도록 이벤트를 먼저 발생
		EnemyCharacter->NotifyHitGaugeThresholdReached();

		// 요구사항에 따라 임계 이벤트 발생 후 게이지를 0으로 초기화
		ResetHitGauge();
	}
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

void UNSMonsterAttributeSet::ReportDamageSenseEvent(const FGameplayEffectModCallbackData& Data) const
{
	const float RawDamage = GetDamage();

	AActor* DamagedActor = Data.Target.GetAvatarActor();
	AActor* InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();

	if (RawDamage <= 0.0f ||
		!DamagedActor ||
		!InstigatorActor)
	{
		return;
	}

	AActor* PerceivedActor = ResolvePerceivedInstigator(InstigatorActor);

	if (!PerceivedActor || !DamagedActor->GetWorld())
	{
		return;
	}

	UAISense_Damage::ReportDamageEvent(
		DamagedActor->GetWorld(),
		DamagedActor,
		PerceivedActor,
		RawDamage,
		PerceivedActor->GetActorLocation(),
		DamagedActor->GetActorLocation());
}

void UNSMonsterAttributeSet::HandleHitGaugeAfterDamage(ANSEnemyCharacterBase* EnemyCharacter, float PreviousHealth)
{
	if (!EnemyCharacter)
	{
		return;
	}

	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);

	// 완전히 방어된 공격이나 사망타는 피격 게이지에 포함하지 않음
	if (AppliedHealthDamage <= 0.0f || GetHealth() <= 0.0f)
	{
		return;
	}

	AccumulateHitGauge(EnemyCharacter);
}

void UNSMonsterAttributeSet::HandleDeathAfterEffect(ANSEnemyCharacterBase* EnemyCharacter) const
{
	if (!EnemyCharacter || GetHealth() > 0.0f)
	{
		return;
	}

	EnemyCharacter->Die();
}

AActor* UNSMonsterAttributeSet::ResolvePerceivedInstigator(AActor* InstigatorActor) const
{
	if (!InstigatorActor)
	{
		return nullptr;
	}

	if (ANSBaseCompanionAI* AttackingDrone = Cast<ANSBaseCompanionAI>(InstigatorActor))
	{
		if (AActor* OwnerPlayer = AttackingDrone->GetOwnerPlayer())
		{
			return OwnerPlayer;
		}
	}

	return InstigatorActor;
}

void UNSMonsterAttributeSet::ExecuteDamageFlashCueAfterDamage(
	const FGameplayEffectModCallbackData& Data,
	float PreviousHealth) const
{
	ANSEnemyCharacterBase* EnemyCharacter = Cast<ANSEnemyCharacterBase>(Data.Target.GetAvatarActor());

	if (!EnemyCharacter ||
		!EnemyCharacter->HasAuthority() ||
		EnemyCharacter->IsInPool())
	{
		return;
	}

	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);

	if (AppliedHealthDamage <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.RawMagnitude = AppliedHealthDamage;
	CueParameters.EffectContext = Data.EffectSpec.GetContext();

	Data.Target.ExecuteGameplayCue(NSGameplayTags::GameplayCue_Damage_Flash, CueParameters);
}

void UNSMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	const bool bIsDamageExecution = Data.EvaluatedData.Attribute == GetDamageAttribute();
	const float PreviousHealth = GetHealth();
	ANSEnemyCharacterBase* EnemyCharacter = Cast<ANSEnemyCharacterBase>(Data.Target.GetAvatarActor());

	// 부모 AttributeSet이 Damage Attribute를 소비하기 전에 처리
	if (bIsDamageExecution)
	{
		ReportDamageSenseEvent(Data);
	}

	// Defense 적용, Damage 초기화, 실제 Health 차감을 처리
	Super::PostGameplayEffectExecute(Data);
	
	if (bIsDamageExecution)
	{
		ExecuteDamageFlashCueAfterDamage(Data, PreviousHealth);
		HandleHitGaugeAfterDamage(EnemyCharacter, PreviousHealth);
	}

	HandleDeathAfterEffect(EnemyCharacter);
}

// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"
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
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
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
	}// ---@ 민재 : 아래 쉴드 옵션 추가---
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	// --- 
}

void UNSMonsterAttributeSet::ResetHitGauge()
{
	SetHitGauge(0.0f);
}

void UNSMonsterAttributeSet::AccumulateHitGauge(UNSEnemyStateComponent* EnemyState)
{
	if (!EnemyState || !EnemyState->CanReceiveHitGauge())
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
		// 피격 게이지 최대치 도달 시 Hit Reaction GA 실행 시도 
		EnemyState->StartHitReaction();
		
		// 현재 피격 게이지를 0으로 초기화
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

void UNSMonsterAttributeSet::HandleHitGaugeAfterDamage(
	UNSEnemyStateComponent* EnemyState,
	float PreviousHealth)
{
	if (!EnemyState)
	{
		return;
	}

	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);

	// 완전히 방어된 공격이나 사망타는 피격 게이지에 포함하지 않음
	if (AppliedHealthDamage <= 0.0f || GetHealth() <= 0.0f)
	{
		return;
	}

	AccumulateHitGauge(EnemyState);
}

void UNSMonsterAttributeSet::HandleDeathAfterEffect(UNSEnemyStateComponent* EnemyState) const
{
	if (!EnemyState || GetHealth() > 0.0f)
	{
		return;
	}

	EnemyState->Die();
}

AActor* UNSMonsterAttributeSet::ResolvePerceivedInstigator(AActor* InstigatorActor) const
{
	if (!InstigatorActor)
	{
		return nullptr;
	}

	if (ANSCompanionDroneAI* AttackingDrone = Cast<ANSCompanionDroneAI>(InstigatorActor))
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
	UNSEnemyStateComponent* EnemyState = GetTargetEnemyState(Data);
	AActor* AvatarActor = Data.Target.GetAvatarActor();

	if (!EnemyState ||
		!AvatarActor ||
		!AvatarActor->HasAuthority() ||
		EnemyState->IsInactive())
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
	UNSEnemyStateComponent* EnemyState = GetTargetEnemyState(Data);

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
		HandleHitGaugeAfterDamage(EnemyState, PreviousHealth);
	}

	HandleDeathAfterEffect(EnemyState);
}

UNSEnemyStateComponent* UNSMonsterAttributeSet::GetTargetEnemyState(
	const FGameplayEffectModCallbackData& Data) const
{
	AActor* AvatarActor = Data.Target.GetAvatarActor();
	return AvatarActor ? AvatarActor->FindComponentByClass<UNSEnemyStateComponent>() : nullptr;
}

// @민재 : 쉴드관련 함수 추가
float UNSMonsterAttributeSet::HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data)
{
	const float CurrentShield = GetShield();
	if (CurrentShield <= 0)
	{
		return DamageAmount;
	}
	
	const float AbsorbedDamage = FMath::Min(CurrentShield, DamageAmount);
	
	SetShield(CurrentShield - AbsorbedDamage);
	
	NotifyHitReaction(Data, ENSHitReactionDamageLayer::Shield, AbsorbedDamage, false);
	
	return DamageAmount - AbsorbedDamage;
}

void UNSMonsterAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, Shield, OldShield);
}

void UNSMonsterAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, MaxShield, OldMaxShield);
}

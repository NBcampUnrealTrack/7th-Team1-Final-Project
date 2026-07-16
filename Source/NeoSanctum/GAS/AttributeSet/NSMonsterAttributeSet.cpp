// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSTurret.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Damage.h"

void UNSMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, HitGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, MaxHitGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, HitGaugeDamageThresholdRatio, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, HitGaugeGainMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, MinHitGaugeGainPerHit, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSMonsterAttributeSet, MaxHitGaugeGainPerHit, COND_None, REPNOTIFY_Always);
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
	else if (Attribute == GetHitGaugeDamageThresholdRatioAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.01f);
	}
	else if (Attribute == GetHitGaugeGainMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMinHitGaugeGainPerHitAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMaxHitGaugeGainPerHitAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	// ---@ 민재 : 아래 쉴드 옵션 추가---
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

void UNSMonsterAttributeSet::AccumulateHitGauge(
	UNSEnemyStateComponent* EnemyState,
	float AppliedHealthDamage)
{
	if (!EnemyState || !EnemyState->CanReceiveHitGauge())
	{
		return;
	}

	const float GaugeGain = CalculateHitGaugeGainFromDamage(AppliedHealthDamage);

	if (GaugeGain <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float GaugeMaximum = FMath::Max(GetMaxHitGauge(), 1.0f);
	const float NewGauge = FMath::Clamp(GetHitGauge() + GaugeGain, 0.0f, GaugeMaximum);

	SetHitGauge(NewGauge);

	if (NewGauge >= GaugeMaximum)
	{
		EnemyState->StartHitReaction();
		ResetHitGauge();
	}
}

float UNSMonsterAttributeSet::CalculateHitGaugeGainFromDamage(float AppliedHealthDamage) const
{
	if (AppliedHealthDamage <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float GaugeMaximum = FMath::Max(GetMaxHitGauge(), 1.0f);
	const float HealthMaximum = FMath::Max(GetMaxHealth(), 1.0f);
	const float ThresholdRatio = FMath::Max(GetHitGaugeDamageThresholdRatio(), 0.01f);
	const float DamageThreshold = FMath::Max(HealthMaximum * ThresholdRatio, 1.0f);

	float GaugeGain = (AppliedHealthDamage / DamageThreshold) * GaugeMaximum;
	GaugeGain *= FMath::Max(GetHitGaugeGainMultiplier(), 0.0f);

	const float MinGain = FMath::Max(GetMinHitGaugeGainPerHit(), 0.0f);
	if (MinGain > 0.0f)
	{
		GaugeGain = FMath::Max(GaugeGain, MinGain);
	}

	const float MaxGain = FMath::Max(GetMaxHitGaugeGainPerHit(), 0.0f);
	if (MaxGain > 0.0f)
	{
		GaugeGain = FMath::Min(GaugeGain, MaxGain);
	}

	return FMath::Max(GaugeGain, 0.0f);
}

void UNSMonsterAttributeSet::OnRep_HitGauge(const FGameplayAttributeData& OldHitGauge)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, HitGauge, OldHitGauge);
}

void UNSMonsterAttributeSet::OnRep_MaxHitGauge(const FGameplayAttributeData& OldMaxHitGauge)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, MaxHitGauge, OldMaxHitGauge);
}

void UNSMonsterAttributeSet::OnRep_HitGaugeDamageThresholdRatio(
	const FGameplayAttributeData& OldHitGaugeDamageThresholdRatio)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNSMonsterAttributeSet,
		HitGaugeDamageThresholdRatio,
		OldHitGaugeDamageThresholdRatio);
}

void UNSMonsterAttributeSet::OnRep_HitGaugeGainMultiplier(
	const FGameplayAttributeData& OldHitGaugeGainMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNSMonsterAttributeSet,
		HitGaugeGainMultiplier,
		OldHitGaugeGainMultiplier);
}

void UNSMonsterAttributeSet::OnRep_MinHitGaugeGainPerHit(
	const FGameplayAttributeData& OldMinHitGaugeGainPerHit)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNSMonsterAttributeSet,
		MinHitGaugeGainPerHit,
		OldMinHitGaugeGainPerHit);
}

void UNSMonsterAttributeSet::OnRep_MaxHitGaugeGainPerHit(
	const FGameplayAttributeData& OldMaxHitGaugeGainPerHit)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNSMonsterAttributeSet,
		MaxHitGaugeGainPerHit,
		OldMaxHitGaugeGainPerHit);
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

	// 실제 Health 피해가 없거나 사망한 경우에는 HitGauge를 누적하지 않음
	if (AppliedHealthDamage <= KINDA_SMALL_NUMBER || GetHealth() <= 0.0f)
	{
		return;
	}

	AccumulateHitGauge(EnemyState, AppliedHealthDamage);
}

void UNSMonsterAttributeSet::HandleDeathAfterEffect(
	UNSEnemyStateComponent* EnemyState,
	const FGameplayEffectModCallbackData& Data) const
{
	if (!EnemyState || GetHealth() > 0.0f)
	{
		return;
	}
	
	AActor* InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();
	AController* Killer = ResolveKillerController(InstigatorActor);

	EnemyState->Die(Killer);
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

AController* UNSMonsterAttributeSet::ResolveKillerController(AActor* InstigatorActor) const
{
	if (!InstigatorActor)
	{
		return nullptr;
	}

	// 터렛은 소환자 Controller
	if (const ANSTurret* Turret = Cast<ANSTurret>(InstigatorActor))
	{
		return Turret->GetOwningController();
	}

	// 드론도 소유 플레이어의 Controller
	if (const ANSCompanionDroneAI* Drone = Cast<ANSCompanionDroneAI>(InstigatorActor))
	{
		if (AActor* OwnerPlayer = Drone->GetOwnerPlayer())
		{
			if (APawn* OwnerPawn = Cast<APawn>(OwnerPlayer))
			{
				return OwnerPawn->GetController();
			}
			return Cast<AController>(OwnerPlayer);
		}
		
		return nullptr;
	}

	// 플레이어가 직접 타격한 경우
	if (const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		return InstigatorPawn->GetController();
	}

	return nullptr; 
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

	HandleDeathAfterEffect(EnemyState, Data);
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

	if (!bOutOfShield && GetShield() <= 0.0f)
	{
		bOutOfShield = true;
		OnOutOfShield.Broadcast();
	}
	
	NotifyHitReaction(Data, ENSHitReactionDamageLayer::Shield, AbsorbedDamage, false);

	return DamageAmount - AbsorbedDamage;
}

void UNSMonsterAttributeSet::ResetOutOfShieldGuard()
{
	bOutOfShield = false;
}

void UNSMonsterAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, Shield, OldShield);
}

void UNSMonsterAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSMonsterAttributeSet, MaxShield, OldMaxShield);
}

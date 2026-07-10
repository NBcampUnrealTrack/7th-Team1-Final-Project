// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSMonsterAttributeSet.generated.h"

class UNSEnemyStateComponent;

/**
 * 몬스터 전용 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSMonsterAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()

public:
	// 피격 게이지 Attribute의 네트워크 복제를 등록하는 함수
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// 피격 게이지 관련 Attribute가 유효 범위를 벗어나지 않게 제한하는 함수
	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) override;

	// 실제 데미지 적용 결과를 기준으로 피격 게이지를 누적하는 함수
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 현재 누적된 피격 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HitGauge, Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData HitGauge;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, HitGauge);

	// 피격 경직 이벤트를 발생시키는 최대 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHitGauge, Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData MaxHitGauge;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, MaxHitGauge);

	// 최대 체력의 이 비율만큼 피해를 받으면 HitGauge가 가득 차도록 계산하는 Attribute
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HitGaugeDamageThresholdRatio,
		Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData HitGaugeDamageThresholdRatio;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, HitGaugeDamageThresholdRatio);

	// 데미지 기반 HitGauge 증가량에 곱하는 보정 배율 Attribute
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HitGaugeGainMultiplier, Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData HitGaugeGainMultiplier;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, HitGaugeGainMultiplier);

	// 유효한 피격 한 번에 보장되는 최소 HitGauge 증가량 Attribute
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MinHitGaugeGainPerHit, Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData MinHitGaugeGainPerHit;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, MinHitGaugeGainPerHit);

	// 유효한 피격 한 번에 증가할 수 있는 최대 HitGauge 증가량 Attribute. 0이면 제한 없음
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHitGaugeGainPerHit, Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData MaxHitGaugeGainPerHit;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, MaxHitGaugeGainPerHit);

	// 현재 피격 게이지를 0으로 초기화하는 함수
	void ResetHitGauge();

	// Target Avatar에서 EnemyStateComponent를 찾는 함수
	UNSEnemyStateComponent* GetTargetEnemyState(const FGameplayEffectModCallbackData& Data) const;

private:
	// 실제 적용된 Health Damage를 기준으로 HitGauge를 누적하는 함수
	void AccumulateHitGauge(UNSEnemyStateComponent* EnemyState, float AppliedHealthDamage);

	// 실제 적용된 Health Damage를 HitGauge 증가량으로 변환하는 함수
	float CalculateHitGaugeGainFromDamage(float AppliedHealthDamage) const;

	// 복제된 현재 피격 게이지를 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_HitGauge(const FGameplayAttributeData& OldHitGauge);

	// 복제된 최대 피격 게이지를 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_MaxHitGauge(const FGameplayAttributeData& OldMaxHitGauge);

	UFUNCTION()
	void OnRep_HitGaugeDamageThresholdRatio(const FGameplayAttributeData& OldHitGaugeDamageThresholdRatio);

	UFUNCTION()
	void OnRep_HitGaugeGainMultiplier(const FGameplayAttributeData& OldHitGaugeGainMultiplier);

	UFUNCTION()
	void OnRep_MinHitGaugeGainPerHit(const FGameplayAttributeData& OldMinHitGaugeGainPerHit);

	UFUNCTION()
	void OnRep_MaxHitGaugeGainPerHit(const FGameplayAttributeData& OldMaxHitGaugeGainPerHit);

private:
	/* PostGameplayEffectExecute 내부 로직 분리 */
	// Damage Attribute가 소비되기 전에 AI Damage Sense 이벤트를 보고하는 함수
	void ReportDamageSenseEvent(const FGameplayEffectModCallbackData& Data) const;

	// 실제 체력 감소량을 확인하고 생존한 몬스터의 피격 게이지를 누적하는 함수
	void HandleHitGaugeAfterDamage(UNSEnemyStateComponent* EnemyState, float PreviousHealth);

	// GameplayEffect 처리 후 체력이 0 이하인 몬스터를 사망시키는 함수
	void HandleDeathAfterEffect(UNSEnemyStateComponent* EnemyState) const;

	// 드론 공격자를 실제 어그로 대상인 소유 플레이어로 변환하는 함수
	AActor* ResolvePerceivedInstigator(AActor* InstigatorActor) const;

private:
	// 실제 체력 피해가 발생했을 때 몬스터 전용 피격 플래시 Cue를 실행하는 함수
	void ExecuteDamageFlashCueAfterDamage(
		const FGameplayEffectModCallbackData& Data,
		float PreviousHealth) const;

	// @민재 : 보스 쉴드 로직을 위한 임시 쉴드 옵션추가
public:
	// TODO(refactor): 현재는 보스만 사용. 향후 UNSBossAttributeSet로 분리 예정 — 아래 Shield 블록만 이전하면 됨
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "GAS|Monster|Shield")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, Shield);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "GAS|Monster|Shield")
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, MaxShield);

protected:
	// Health 차감 전 Shield로 데미지를 흡수 (MaxShield=0인 일반 몬스터는 즉시 통과)
	virtual float HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data) override;

private:
	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield);
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);
};

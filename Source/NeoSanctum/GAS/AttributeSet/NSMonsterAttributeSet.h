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

	// 유효한 피격 한 번에 증가하는 게이지 수치
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HitGaugeGainPerHit, Category = "GAS|Monster|Hit Gauge")
	FGameplayAttributeData HitGaugeGainPerHit;
	ATTRIBUTE_ACCESSORS(UNSMonsterAttributeSet, HitGaugeGainPerHit);

	// 현재 피격 게이지를 0으로 초기화하는 함수
	void ResetHitGauge();

	// Target Avatar에서 EnemyStateComponent를 찾는 함수
	UNSEnemyStateComponent* GetTargetEnemyState(const FGameplayEffectModCallbackData& Data) const;

private:
	// 생존 중인 몬스터의 피격 게이지를 한 번 누적하는 함수
	void AccumulateHitGauge(UNSEnemyStateComponent* EnemyState);

	// 복제된 현재 피격 게이지를 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_HitGauge(const FGameplayAttributeData& OldHitGauge);

	// 복제된 최대 피격 게이지를 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_MaxHitGauge(const FGameplayAttributeData& OldMaxHitGauge);

	// 복제된 피격당 증가량을 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_HitGaugeGainPerHit(const FGameplayAttributeData& OldHitGaugeGainPerHit);
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
};

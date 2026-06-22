// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSMonsterAttributeSet.generated.h"

class ANSEnemyCharacterBase;

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

private:
	// 생존 중인 몬스터의 피격 게이지를 한 번 누적하는 함수
	void AccumulateHitGauge(ANSEnemyCharacterBase* EnemyCharacter);

	// 복제된 현재 피격 게이지를 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_HitGauge(const FGameplayAttributeData& OldHitGauge);

	// 복제된 최대 피격 게이지를 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_MaxHitGauge(const FGameplayAttributeData& OldMaxHitGauge);

	// 복제된 피격당 증가량을 GAS Attribute 시스템에 반영하는 콜백 함수
	UFUNCTION()
	void OnRep_HitGaugeGainPerHit(const FGameplayAttributeData& OldHitGaugeGainPerHit);
};

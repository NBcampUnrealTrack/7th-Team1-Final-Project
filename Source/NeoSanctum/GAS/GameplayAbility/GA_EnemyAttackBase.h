// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EnemyAttackBase.generated.h"

class UNSEnemyData;

/**
 * 모든 적 AI 공격 어빌리티의 최상위 부모 C++ 클래스
 * 멀티플레이어 애니메이션 동기화 및 BT 제어권 연동 흐름을 통제합니다.
 */
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackBase();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnHitCheckEventReceived(FGameplayEventData Payload);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Assets")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Assets")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Assets")
	FGameplayTag GameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Config")
	FGameplayTag HitCheckEventTag;

	// 공격 판정 전방 거리
	float AttackTraceDistance;

	// 공격 판정 구체 반경
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Config")
	float AttackTraceRadius;
	
private:
	TObjectPtr<UNSEnemyData> EnemyData;
};

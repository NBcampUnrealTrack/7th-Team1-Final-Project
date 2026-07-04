// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Dash.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilitySystemComponent;

/**
 * 캐릭터 대시 어빌리티
 */
UCLASS()
class NEOSANCTUM_API UGA_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dash();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

protected:
	// 정해진 대쉬 지속시간 만큼 대쉬를 수행하고 끝나면 어빌리티를 종료하기 위한 콜백
	UFUNCTION()
	void OnDashFinished();

	// 대쉬 정상 종료 직후 Vanguard 대쉬공격 입력 창 열기
	void AddDashAttackWindow();

	// 대쉬공격 입력 가능 창 종료 및 상태태그 제거
	void RemoveDashAttackWindow();

protected:
	// 대쉬 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Dash", meta = (ClampMin = "0.0"))
	float DashDistance = 450.f;

	// 대쉬가 지속되는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Dash", meta = (ClampMin = "0.01"))
	float DashDuration = 0.2f;

	// 대쉬를 허용할 최소 바닥 Normal Z : 1에 가까울 수록 평지 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Dash", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinFloorNormalZ = 0.5f;

	// 대쉬 중 중력 적용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Dash")
	bool bEnableGravityDuringDash = true;

	// 대쉬 종료 후 대쉬공격 입력 허용 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Dash", meta = (ClampMin = "0.0"))
	float DashAttackWindowDuration = 0.45f;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DashTask;

	// 대쉬공격 입력 가능 창 자동 종료 타이머
	FTimerHandle DashAttackWindowTimerHandle;

	// Ability 종료 이후 타이머에서도 대쉬공격 입력 창 태그를 제거하기 위한 ASC 참조
	TWeakObjectPtr<UAbilitySystemComponent> DashAttackWindowASC;
};

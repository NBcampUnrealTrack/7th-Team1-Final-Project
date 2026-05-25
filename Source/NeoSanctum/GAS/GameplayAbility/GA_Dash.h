// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Dash.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;

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
	
protected:
	// 정해진 대쉬 지속시간 만큼 대쉬를 수행하고 끝나면 어빌리티를 종료하기 위한 콜백
	UFUNCTION()
	void OnDashFinished();

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

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DashTask;
};

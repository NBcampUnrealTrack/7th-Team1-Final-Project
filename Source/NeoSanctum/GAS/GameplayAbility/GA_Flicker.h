// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_Flicker.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;
class UGameplayEffect;

/**
 * Crosshair 근처의 사거리 내 적 하나에게 빠르게 접근해 베는 스킬.
 */
UCLASS()
class NEOSANCTUM_API UGA_Flicker : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_Flicker();

protected:
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

private:
	UFUNCTION()
	void OnFlickerMoveFinished();

	// 크로스헤어 기준 최적 타겟 탐색
	bool TryFindBestTarget(AActor*& OutTargetActor, FVector& OutTargetLocation) const;
	// 로컬 조준선 또는 서버 AimRotation 기반 조준선 계산
	bool TryBuildAimRay(FVector& OutRayStart, FVector& OutRayDirection) const;
	// 타겟까지의 시야 확보 여부 확인
	bool HasSightToTarget(const FVector& SightStart, AActor* TargetActor, const FVector& TargetLocation) const;
	// 타겟 앞 공격 위치 계산
	bool TryBuildAttackLocation(AActor* TargetActor, const FVector& TargetLocation, FVector& OutAttackLocation) const;
	
	// 공격 위치로 이동 시작
	bool StartFlickerMove(const FVector& AttackLocation);
	// 이동 전 MovementMode 복구
	void RestoreMovementMode() const;
	
private:	
	// 현재 타겟에게 데미지 적용
	void ApplyDamageToTarget();
	// DamageCoefficient 기반 최종 데미지 계산
	bool TryGetFinalDamage(float& OutDamage) const;
	
private:
	// Dashing 상태 태그 부여
	void AddDashingState();
	// Dashing 상태 태그 제거
	void RemoveDashingState();

private:
	// Flicker 데미지 GameplayEffect
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 타겟 앞 위치까지 이동하는 시간
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.01"))
	float MoveDuration = 0.16f;

	// 크로스헤어 기준 타겟 허용 각도
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxTargetAngleDegrees = 20.0f;

	// 타겟과 유지할 공격 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.0"))
	float AttackDistance = 150.0f;

	// 타겟 시야 확인 Trace 채널
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = NSCollisionChannels::CombatSight;

	// 돌진 중 State.Dashing 태그 사용 여부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker")
	bool bUseDashingStateTag = true;

	// 공격 위치 이동 AbilityTask
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> MoveTask;

	// 현재 공격 대상
	TWeakObjectPtr<AActor> CurrentTarget;
	// 현재 공격 대상 위치
	FVector CurrentTargetLocation = FVector::ZeroVector;
	// 이동 전 MovementMode
	TOptional<TEnumAsByte<EMovementMode>> PreviousMovementMode;
};

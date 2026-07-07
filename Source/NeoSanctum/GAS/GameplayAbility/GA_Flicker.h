// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_Flicker.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class ANSMeleeWeapon;

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

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
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

	UFUNCTION()
	void OnFlickerMontageCompleted();

	UFUNCTION()
	void OnFlickerMontageInterrupted();

	UFUNCTION()
	void OnFlickerHitEventReceived(FGameplayEventData Payload);

	// Flicker 몽타주 재생 시작
	bool PlayFlickerMontage();
	// Flicker 몽타주 정지
	void StopFlickerMontage() const;
	// Flicker Hit 이벤트 대기 시작
	void StartHitEventTask();
	// 현재 타격 순서에 맞는 Attack 섹션으로 몽타주 전환
	bool JumpToAttackSection() const;
	// Trace 이벤트 처리
	void HandleMeleeHitEvent(const FGameplayEventData& Payload);
	// 현재 장착 중인 근접 무기 조회
	ANSMeleeWeapon* GetCurrentMeleeWeapon() const;
	// 근접 무기 소켓 궤적 Sweep 수행
	void PerformMeleeSocketSweeps(ANSMeleeWeapon& MeleeWeapon);
	// 단일 소켓 궤적 Sweep 수행
	void SweepMeleeTrace(const FVector& TraceStart, const FVector& TraceEnd);

	// 크로스헤어 기준 최적 타겟 탐색
	bool TryFindBestTarget(AActor*& OutTargetActor, FVector& OutTargetLocation) const;
	// Primary Target 기준 체인 타겟 구성
	bool TryBuildTargetChain(AActor* PrimaryTarget, const FVector& PrimaryTargetLocation);
	// CombatStat.HitCount 기준 타격 수 조회
	bool TryGetHitCount(int32& OutHitCount) const;

	// 로컬 조준선 또는 서버 AimRotation 기반 조준선 계산
	bool TryBuildAimRay(FVector& OutRayStart, FVector& OutRayDirection) const;
	// 타겟까지의 시야 확보 여부 확인
	bool HasSightToTarget(const FVector& SightStart, AActor* TargetActor, const FVector& TargetLocation) const;
	// 타겟 앞 공격 위치 계산
	bool TryBuildAttackLocation(AActor* TargetActor, const FVector& TargetLocation, FVector& OutAttackLocation) const;
	bool IsAttackableTarget(AActor* TargetActor) const;

	// 현재 체인 타겟 돌진 시작
	bool StartCurrentTargetMove();
	// 다음 체인 타겟 처리
	void AdvanceToNextTarget();
	// Attack 섹션 종료 시점까지 대기 후 다음 타겟 처리
	void ScheduleAdvanceToNextTargetAfterAttackSection();
	// 현재 타격 순서에 맞는 Attack 섹션 이름 조회
	FName GetCurrentAttackSectionName() const;
	// 현재 Attack 섹션의 남은 재생 시간 조회
	float GetCurrentAttackSectionRemainingTime() const;
	// 공격 위치로 이동 시작
	bool StartFlickerMove(const FVector& AttackLocation);
	// 이동 전 MovementMode 복구
	void RestoreMovementMode();

	// 현재 타겟에게 데미지 적용
	void ApplyDamageToTarget();
	// 근접 무기 Trace 결과 기반 데미지 적용
	bool ApplyDamageToActor(const FHitResult& HitResult);
	// DamageCoefficient 기반 최종 데미지 계산
	bool TryGetFinalDamage(float& OutDamage) const;

	// Dashing 상태 태그 부여
	void AddDashingState();
	// Dashing 상태 태그 제거
	void RemoveDashingState();
	void AddFlickerGameplayCue();
	void RemoveFlickerGameplayCue();

private:
	// Flicker 데미지 GameplayEffect
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	// Flicker 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker|Montage")
	TObjectPtr<UAnimMontage> FlickerMontage;

	// Flicker 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker|Montage", meta = (ClampMin = "0.01"))
	float FlickerMontagePlayRate = 1.0f;

	// 돌진 공격 단계의 애니메이션 몽타주 섹션 이름 목록
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker|Montage")
	TArray<FName> AttackSectionNames;

	// 데미지 적용 타이밍을 전달하는 GameplayEvent 태그
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker|Montage")
	FGameplayTag HitEventTag;

	// 타겟 앞 위치까지 이동하는 시간
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.01"))
	float MoveDuration = 0.16f;

	// 크로스헤어 기준 타겟 허용 각도
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxTargetAngleDegrees = 20.0f;

	// 타겟과 유지할 공격 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.0"))
	float AttackDistance = 100.0f;

	// Primary Target 주변 추가 타겟 탐색 반경
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker", meta = (ClampMin = "0.0"))
	float ChainRadius = 1000.0f;

	// 타겟 시야 확인 Trace 채널
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = NSCollisionChannels::CombatSight;

	// 근접 무기 궤적 Sweep 반경
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker|Trace", meta = (ClampMin = "0.0"))
	float MeleeTraceRadius = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker|Cue")
	FGameplayTag FlickerGameplayCueTag;

	// 돌진 중 State.Dashing 태그 사용 여부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Flicker")
	bool bUseDashingStateTag = true;

	// 공격 위치 이동 AbilityTask
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> MoveTask;

	// Flicker 몽타주 재생 AbilityTask
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	// Flicker Hit GameplayEvent 대기 Task
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> HitEventTask;

	// Attack 섹션 종료 대기 타이머
	FTimerHandle AttackAdvanceTimerHandle;

	// Release 입력 처리 여부
	bool bReleaseRequested = false;
	// 돌진 시작 여부
	bool bDashStarted = false;
	bool bFlickerGameplayCueAdded = false;
	// 현재 타겟 데미지 적용 여부
	bool bCurrentTargetDamageApplied = false;
	// 직전 Trace Tick의 무기 소켓 위치 목록
	TArray<FVector> PreviousMeleeTraceSocketLocations;
	// 직전 Trace Tick의 무기 소켓 위치 유효 여부
	bool bHasPreviousMeleeTraceSocketLocations = false;
	// 현재 무기 Trace NotifyState 식별자
	uint32 CurrentMeleeTraceWindowId = 0;
	// 현재 Trace Window에서 이미 데미지를 적용한 대상 목록
	TSet<TObjectKey<AActor>> DamagedActorsInTraceWindow;

	// 현재 공격 대상
	TWeakObjectPtr<AActor> CurrentTarget;
	// 현재 공격 대상 위치
	FVector CurrentTargetLocation = FVector::ZeroVector;
	// 이동 전 MovementMode
	TOptional<TEnumAsByte<EMovementMode>> PreviousMovementMode;
	// 이번 발동에서 공격할 타겟 목록
	TArray<TWeakObjectPtr<AActor>> SelectedTargets;
	// 이번 발동에서 공격할 타겟 위치 목록
	TArray<FVector> SelectedTargetLocations;
	// 현재 처리 중인 체인 타겟 인덱스
	int32 CurrentTargetIndex = INDEX_NONE;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "NSEnemyThreatComponent.generated.h"

struct FNSEnemyThreatDamage
{
	double Timestamp = 0.0;
	float Damage = 0.0f;
};

struct FNSEnemyThreatRecord
{
	TWeakObjectPtr<AActor> TargetActor;
	TArray<FNSEnemyThreatDamage> DamageSamples;
	FVector LastKnownLocation = FVector::ZeroVector;
	double LastSeenTime = -1.0;
	double LastStimulusTime = -1.0;
	bool bCurrentlyVisible = false;
};

struct FNSEnemyThreatUpdateResult
{
	TWeakObjectPtr<AActor> PreviousTarget;
	TWeakObjectPtr<AActor> CurrentTarget;
	bool bTargetChanged = false;
};

/**
 * Enemy의 Threat 기록, 어그로 타겟 선택, 타겟 전환 판단을 담당하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyThreatComponent();

	// 타겟 평가 주기가 되었는지 확인하는 함수
	bool CanEvaluateTarget() const;

	// 다음 타겟 평가 시간을 예약하는 함수
	void ScheduleNextEvaluation();

	// Threat, 현재 타겟, 타이머 상태를 초기화하는 함수
	void ResetThreatState();

	// 현재 타겟을 향해 공격이 시작됐음을 기록하는 함수
	void NotifyAttackStarted();

	// 감지 정보를 Threat 기록에 반영하는 함수
	void UpdateThreatFromStimulus(AActor* Actor, const FAIStimulus& Stimulus);

	// 현재 Threat 상태를 평가해 유지, 변경, 해제를 결정하는 함수
	FNSEnemyThreatUpdateResult UpdateTarget(bool bIsAttacking, bool bCanMaintainCurrentTarget);

	// 특정 타겟의 Threat 기록을 제거하고 필요하면 현재 타겟도 해제하는 함수
	void RemoveTarget(AActor* TargetActor, bool bClearIfCurrent);

	// 현재 선택된 전투 타겟을 반환하는 함수
	AActor* GetCurrentTarget() const;
	
	// 현재 Threat 기록에 남아 있는 유효한 타겟 목록을 반환하는 함수
	void GetKnownTargets(TArray<AActor*>& OutTargets, bool bOnlyVisible = false) const;

	// 현재 타겟을 해제하고 필요하면 재획득 쿨다운을 적용하는 함수
	void ClearCurrentTarget(bool bBlockReacquisition);

	// 타겟의 마지막 감지 위치를 가져오는 함수
	bool TryGetLastKnownLocation(const AActor* TargetActor, FVector& OutLocation) const;

	// 특정 타겟에게 받은 가장 최근 피해 시간을 반환하는 함수
	double GetLatestDamageTime(AActor* TargetActor) const;

protected:
	// 타겟 선택 로직을 다시 평가하는 최소 간격
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.05"))
	float TargetEvalInterval = 0.25f;

	// 시야에서 사라진 타겟을 계속 기억하는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.0"))
	float SightMemoryDuration = 2.0f;

	// 청각/피해 감지 정보를 유지하는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.0"))
	float StimulusMemoryDuration = 3.0f;

	// 누적 피해 Threat 계산에 포함할 최근 시간 범위
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.1"))
	float DamageThreatWindow = 10.0f;

	// 새 타겟 선택 직후 타겟 변경을 막는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.0"))
	float InitialTargetLockDuration = 2.5f;

	// 타겟 변경 후 다시 변경할 수 있을 때까지의 시간
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.0"))
	float TargetSwitchCooldown = 2.0f;

	// 새 타겟의 피해 Threat가 기존 타겟보다 얼마나 높아야 전환할지 기준
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "1.0"))
	float DamageThreatSwitchRatio = 1.25f;

	// 피해 기록이 없을 때 새 타겟이 얼마나 더 가까워야 전환할지 기준
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DistanceSwitchRatio = 0.7f;

	// 공격하지 못한 상태로 타겟을 추적할 수 있는 최대 시간
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.1"))
	float MaxPursuitWithoutAttackDuration = 10.0f;

	// 추적 포기한 타겟을 다시 선택하지 않는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Threat", meta = (ClampMin = "0.0"))
	float TargetReacquireCooldown = 2.0f;

private:
	void PruneThreatRecords(double CurrentTime, bool bCanMaintainCurrentTarget);
	AActor* FindBestTarget(double CurrentTime) const;
	float GetRecentDamageThreat(const FNSEnemyThreatRecord& Record, double CurrentTime) const;
	bool IsThreatRecordRelevant(const FNSEnemyThreatRecord& Record, double CurrentTime) const;
	bool ShouldSwitchTarget(AActor* CandidateTarget, double CurrentTime) const;
	bool IsValidLivingTarget(const AActor* Target) const;
	void SetCurrentTarget(AActor* NewTarget);

private:
	TMap<TWeakObjectPtr<AActor>, FNSEnemyThreatRecord> ThreatRecords;
	TMap<TWeakObjectPtr<AActor>, double> ReacquireBlockedUntil;

	TWeakObjectPtr<AActor> CurrentTarget;

	double CurrentTargetSelectedTime = 0.0;
	double LastTargetSwitchTime = 0.0;
	double LastCombatProgressTime = 0.0;
	double NextTargetEvalTime = 0.0;

	bool bAttackStartedOnCurrentTarget = false;
};

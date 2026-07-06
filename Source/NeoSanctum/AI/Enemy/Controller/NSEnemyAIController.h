// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSEnemyControllerBase.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "NSEnemyAIController.generated.h"

class UAIPerceptionComponent;
class UNSEnemyData;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UNSEnemyMeleeComponent;
class UNSEnemyMoveComponent;

struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.05.25
 * 
 * 클래스 개요 : 일반 Enemy Character AI를 제어하는 AI Controller
 * Threat, Target, Attack, Melee, Move 컴포넌트를 조합해 일반 몬스터의 타깃 선택, 공격 선택, 근접 예약, 후퇴, 피격 상태를 처리
*/
UCLASS()
class NEOSANCTUM_API ANSEnemyAIController : public ANSEnemyControllerBase
{
	GENERATED_BODY()

public:
	ANSEnemyAIController();

	virtual void Tick(float DeltaTime) override;

	// AI Controller가 Enemy 팀으로 인식되도록 TeamId를 반환하는 함수
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// 감지한 대상과의 적대/우호 관계를 판정하는 함수
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

public:
	// 현재 조건에서 사용할 공격 Row를 선택하는 함수
	const FNSEnemyAttackRow* GetAttackRowByDistance();

	// 현재 조건에서 사용 가능한 공격 Row가 하나라도 있는지 확인하는 함수
	bool CanUseAnyAttackByDistance();

	// 공격이 실제로 실행됐을 때 쿨다운과 Threat 상태를 기록하는 함수
	void RecordAttackUsed(const FNSEnemyAttackRow& AttackRow);

	// 현재 대표 전투 타깃을 반환하는 함수
	AActor* GetCurrentTargetActor() const;

	// 현재 실제 공격 대상 Actor를 반환하는 함수
	AActor* GetCurrentAttackActor() const;

	// 현재 타깃을 향한 공격이 시작됐음을 기록하고 추적 제한 시간을 갱신하는 함수
	virtual void NotifyAttackStarted() override;

	// 공격 Ability 종료, 실패, 취소 이후 일반 Enemy 공격 상태를 정리하는 함수
	virtual void NotifyAttackFinished() override;

public:
	// 현재 전투 타깃에 근접 공격 예약을 요청하고 요청 처리 가능 여부를 반환하는 함수
	bool RequestMeleeAttackReservation();

	// 현재 몬스터가 타깃의 활성 근접 공격 예약을 보유하고 있는지 확인하는 함수
	bool HasMeleeAttackReservation() const;

	// 예약이 필요 없거나 활성 예약을 보유하여 현재 타깃에게 접근할 수 있는지 확인하는 함수
	bool CanApproachMeleeTarget() const;

	// 현재 타깃에 예약 컴포넌트가 있고 이 몬스터가 근접 예약 대상인지 확인하는 함수
	bool CurrentTargetRequiresMeleeReservation() const;

	// 실제 공격 시작을 예약 컴포넌트에 알려 예약 단계를 공격 중으로 전환하는 함수
	void NotifyMeleeReservationAttackStarted();

	// 현재 활성 예약과 대기 요청을 반환하고 선택적으로 재획득 쿨다운을 적용하는 함수
	void ReleaseMeleeAttackReservation(bool bStartReacquireCooldown = true);

public:
	// 피격 경직이 시작되면 이동과 공격을 중단하고 근접 예약을 유지하는 함수
	void HandleHitReactionStarted();

	// 피격 경직이 끝나면 Blackboard와 근접 예약 상태를 갱신하는 함수
	void HandleHitReactionFinished();

protected:
	// Possess 시점에 EnemyData 기준 Brain과 Blackboard 상태를 초기화하는 함수
	virtual void OnPossess(APawn* InPawn) override;

	// UnPossess 시점에 Brain과 런타임 타깃 상태를 정리하는 함수
	virtual void OnUnPossess() override;

	// PerceptionComponent가 감지 정보를 갱신했을 때 Threat 기록을 갱신하는 함수
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	// 현재 Possess 중인 Enemy의 AttackComponent를 반환하는 함수
	UNSEnemyAttackComponent* GetEnemyAttackComponent() const;

	// 현재 Possess 중인 Enemy의 TargetComponent를 반환하는 함수
	UNSEnemyTargetComponent* GetEnemyTargetComponent() const;

	// 현재 Possess 중인 Enemy의 ThreatComponent를 반환하는 함수
	UNSEnemyThreatComponent* GetEnemyThreatComponent() const;

	// 현재 Possess 중인 Enemy의 MeleeComponent를 반환하는 함수
	UNSEnemyMeleeComponent* GetEnemyMeleeComponent() const;

	// 현재 Possess 중인 Enemy의 MoveComponent를 반환하는 함수
	UNSEnemyMoveComponent* GetEnemyMoveComponent() const;

private:
	// 대상이 체력 데이터를 갖고 있고 살아 있는지 확인하는 함수
	bool IsValidLivingTarget(const AActor* Target) const;

	// 현재 조건에서 공격 Row를 찾고 필요하면 가중치 선택을 수행하는 함수
	const FNSEnemyAttackRow* FindAttackRowByDistance(bool bSelectWeightedAttack);

private:
	// 현재 Threat 정보를 평가해 타깃 선택, 유지, 전환, 해제를 처리하는 함수
	void UpdateTargetSelection();

	// 현재 타깃과 관련 Blackboard 값을 제거하고 필요하면 재선택을 잠시 차단하는 함수
	void ClearCurrentCombatTarget(bool bBlockReacquisition);

	// Threat, 타깃, 타이머 등 Controller의 모든 타깃 관리 상태를 초기화하는 함수
	void ResetTargetingState();

	// 현재 타깃, 마지막 위치, 시야 여부를 Blackboard에 반영하는 함수
	void UpdateCurrentTargetBlackboard();

	// 현재 타깃이 파괴 가능한 엄폐물 뒤에 있으면 추적 포기 타이머를 멈출 수 있는지 확인하는 함수
	bool CanMaintainCoverAttackTarget(AActor* TargetActor) const;

private:
	// 현재 타깃과 예약 상태를 확인하여 예약 요청 및 Blackboard 값을 갱신하는 함수
	void UpdateMeleeReservationState();

	// 예약 중인 타깃에 대한 활성 예약 또는 대기 요청을 취소하고 상태를 초기화하는 함수
	void CancelMeleeReservationRequest(bool bStartReacquireCooldown);

	// 현재 공격 데이터 기준으로 근접 공격 예약을 사용하는지 확인하는 함수
	bool UsesMeleeAttackReservation() const;

	// 현재 타깃으로부터 마지막으로 피해를 받은 시간을 Threat 기록에서 가져오는 함수
	double GetLatestDamageTimeFromCurrentTarget() const;

	// 근접 예약 보유 및 접근 가능 상태를 Blackboard에 기록하는 함수
	void SetMeleeReservationBlackboard(bool bHasReservation, bool bCanApproach);

private:
	// EnemyData의 EQS Query를 Blackboard에 등록하고 런타임 상태를 초기화하는 함수
	void InitializeMeleeEQSBlackboard(const UNSEnemyData* EnemyData);

	// 현재 타깃이 변경됐을 때 이전 EQS 결과를 제거하고 재탐색을 요청하는 함수
	void ResetMeleeEQSForCurrentTarget();

private:
	// 추적, 공격, 후퇴 상태에 따라 이동 방향 회전과 타깃 방향 회전을 전환하는 함수
	void UpdateFacingMode(AActor* TargetActor);

	// 후퇴 조건과 후퇴 Blackboard 상태를 갱신하는 함수
	void UpdateRetreatState(AActor* TargetActor);

private:
	// 플레이어까지 Trace해서 실제로 공격할 Actor를 계산하는 함수
	AActor* ResolveAttackActor(AActor* TargetActor, bool& bOutHasDirectLineOfSight) const;

	// Blackboard의 AttackActor 키를 갱신하는 함수
	void SetAttackActorBlackboard(AActor* AttackActor);

private:
	// Possess 시 기본 Blackboard 상태를 초기화하는 함수
	void InitBBState();

	// 공격 진행 상태를 초기화하는 함수
	void ClearAttackBB();

	// 타깃 관련 Blackboard 값을 초기화하는 함수
	void ClearTargetBB(bool bClearCanAttack);

	// 후퇴 관련 Blackboard 값을 초기화하는 함수
	void ClearRetreatBB();

protected:
	// 시야/청각 감지 정보를 수신하는 AI Perception 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	// 타깃을 정면으로 바라봤다고 판정할 최대 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AttackFacingAngleDegrees = 2.0f;
};

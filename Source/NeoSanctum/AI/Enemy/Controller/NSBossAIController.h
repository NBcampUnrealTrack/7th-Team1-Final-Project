// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSEnemyControllerBase.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "NSBossAIController.generated.h"

class UAIPerceptionComponent;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UNSEnemyStateComponent;
class UNSBossModeComponent;
class UNSBossTargetComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.03
 * 
 * 클래스 개요 : Boss Pawn을 제어하는 공통 AI Controller
 * EnemyData의 BrainType에 따라 BehaviorTree 또는 StateTree를 실행하고,
 * Threat, Attack, Phase, Mode, BossTarget 컴포넌트를 조합해 보스 전투 판단을 수행
*/
UCLASS()
class NEOSANCTUM_API ANSBossAIController : public ANSEnemyControllerBase
{
	GENERATED_BODY()

public:
	ANSBossAIController();

	virtual void Tick(float DeltaTime) override;

	// Boss Controller가 Enemy 팀으로 인식되도록 TeamId를 반환하는 함수
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// 감지한 대상과의 적대/우호 관계를 판정하는 함수
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

	// 현재 조건에서 사용할 공격 Row를 선택하는 함수
	const FNSEnemyAttackRow* GetAttackRowByDistance();
	
	// 지정 AttackId와 현재 조건이 일치할 때 사용할 공격 Row를 선택하는 함수
	const FNSEnemyAttackRow* GetAttackRowById(FName AttackId);

	// 현재 조건에서 사용 가능한 공격 Row가 하나라도 있는지 확인하는 함수
	bool CanUseAnyAttackByDistance();
	
	// 지정 AttackId가 현재 조건에서 사용 가능한지 부작용 없이 확인하는 함수
	virtual bool CanUseAttackById(FName AttackId);

	// Boss가 현재 공격 실행 중인지 확인하는 함수
	virtual bool IsBossAttackInProgress() const;

	// 공격이 실제로 실행됐을 때 쿨다운과 Threat 상태를 기록하는 함수
	void RecordAttackUsed(const FNSEnemyAttackRow& AttackRow);
	
	// 공격 Ability 활성화 성공 후 쿨다운과 Threat 시작 시간을 기록하는 함수
	void RecordAttackCommitted(const FNSEnemyAttackRow& AttackRow);

	// 공격 Ability 종료, 실패, 취소 이후 Boss 전용 공격 상태를 정리하는 함수
	virtual void NotifyAttackFinished() override;

	// ThreatComponent가 선택한 대표 전투 타깃을 반환하는 함수
	AActor* GetCurrentTargetActor() const;

	// 현재 공격 판정에 사용할 실제 공격 대상 Actor를 반환하는 함수
	AActor* GetCurrentAttackActor() const;

	// 이번 공격에서 실제로 사용할 다중 타깃 목록을 반환하는 함수
	void GetCurrentAttackTargets(TArray<AActor*>& OutTargets) const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// PerceptionComponent가 감지 정보를 갱신했을 때 Threat 기록을 갱신하는 함수
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	// Boss가 현재 공격 후보를 갱신할 수 있는지 확인하는 함수
	virtual bool CanUpdateAttackAvailability() const;

private:
	// Boss AI가 현재 판단과 공격을 멈춰야 하는 상태인지 확인하는 함수
	bool IsBossAIBlocked() const;

	// 대상이 체력 데이터를 갖고 있고 살아 있는지 확인하는 함수
	bool IsValidLivingTarget(const AActor* Target) const;

	// 현재 타깃을 유지할 수 있는지 확인하는 함수
	bool CanMaintainCurrentTarget(AActor* TargetActor) const;
	
	// 지정 AttackId 기준으로 현재 조건에서 공격 가능 여부만 검사하는 함수
	bool CanUseAttackRowById(FName AttackId) const;

	// 현재 조건에서 공격 Row를 찾고 공격 대상 상태를 갱신하는 함수
	const FNSEnemyAttackRow* FindAttackRowByDistance(bool bSelectWeightedAttack);
	
	// 지정 AttackId 기준으로 현재 조건에서 사용할 공격 Row를 찾는 함수
	const FNSEnemyAttackRow* FindAttackRowById(FName AttackId);

	// 선택된 AttackRow의 TargetPolicy 기준으로 공격 타깃 목록을 구성하는 함수
	bool BuildAttackTargetsForRow(const FNSEnemyAttackRow& AttackRow);

	// ThreatComponent 기준으로 현재 타깃 선택을 갱신하는 함수
	void UpdateTargetSelection();

	// Threat, 공격 타깃, Blackboard 타깃 상태를 초기화하는 함수
	void ResetTargetingState();

	// 현재 전투 타깃을 해제하고 Blackboard 상태를 정리하는 함수
	void ClearCurrentCombatTarget(bool bBlockReacquisition);

	// 현재 타깃 정보를 Blackboard에 반영하는 함수
	void UpdateCurrentTargetBlackboard();

	// 현재 ModeTag를 Blackboard에 반영하는 함수
	void SyncModeBlackboard();

	// 공격 대상 Actor를 런타임 변수와 Blackboard에 반영하는 함수
	void SetAttackActorState(AActor* AttackActor);

	// 공격 관련 런타임 상태와 Blackboard 값을 초기화하는 함수
	void ClearAttackState();

	// 타깃 관련 Blackboard 값을 초기화하는 함수
	void ClearTargetBB(bool bClearCanAttack);

	// Possess 시 Blackboard 기본값을 초기화하는 함수
	void InitBBState();

	// 현재 Possess 중인 Pawn의 AttackComponent를 반환하는 함수
	UNSEnemyAttackComponent* GetEnemyAttackComponent() const;

	// 현재 Possess 중인 Pawn의 TargetComponent를 반환하는 함수
	UNSEnemyTargetComponent* GetEnemyTargetComponent() const;

	// 현재 Possess 중인 Pawn의 ThreatComponent를 반환하는 함수
	UNSEnemyThreatComponent* GetEnemyThreatComponent() const;

	// 현재 Possess 중인 Pawn의 StateComponent를 반환하는 함수
	UNSEnemyStateComponent* GetEnemyStateComponent() const;

	// 현재 Possess 중인 Pawn의 BossModeComponent를 반환하는 함수
	UNSBossModeComponent* GetBossModeComponent() const;

	// 현재 Possess 중인 Pawn의 BossTargetComponent를 반환하는 함수
	UNSBossTargetComponent* GetBossTargetComponent() const;

private:
	// 시야/청각 감지 정보를 수신하는 AI Perception 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	// Blackboard가 없는 StateTree에서도 현재 공격 대상 Actor를 보관하는 변수
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentAttackActor;
};

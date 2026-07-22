// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeEvaluatorBase.h"
#include "NeoSanctum/Combat/Component/NSBossAbilityExecutorComponent.h"
#include "NSSTEvaluator_BossCombat.generated.h"

class ANSBossAIController;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.05
 * 
 * 클래스 개요 : Boss StateTree에서 사용할 전투 상태 값을 계산하고 StateTree 내부 데이터로 제공하는 Evaluator
*/
USTRUCT()
struct FNSSTEvaluator_BossCombatInstanceData
{
	GENERATED_BODY()

	// 현재 Boss가 대표 전투 대상으로 추적하는 Actor
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 현재 Boss 공격이 실제로 조준하거나 타격할 Actor
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> AttackActor = nullptr;

	// 현재 TargetActor가 유효한지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasTarget = false;

	// 현재 AttackActor가 유효한지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasAttackActor = false;

	// 현재 TargetActor에게 직접 시야가 닿는지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasLineOfSight = false;

	// Boss와 TargetActor 사이의 거리
	UPROPERTY(VisibleAnywhere, Category = "Output")
	float TargetDistance = 0.0f;

	// Boss가 사망 상태인지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bIsDead = false;

	// Boss가 비활성 상태인지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bIsInactive = false;

	// Boss가 피격 경직 또는 그로기 상태인지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bIsHitReacting = false;

	// Boss가 Phase 전환 패턴 잠금 상태인지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bIsPhaseLocked = false;

	// Boss가 공격 Ability 실행 또는 RecoverTime 처리 중인지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bIsAttacking = false;

	// Boss가 현재 행동 또는 공격 선택을 시작할 수 있는지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bCanAct = false;

	// Boss가 현재 공격 선택 조건 검사를 시도할 수 있는지 여부
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bCanSelectAttack = false;

	// 현재 PhaseId
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FName CurrentPhaseId = NAME_None;

	// 현재 PhaseTag
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FGameplayTag CurrentPhaseTag;

	// 현재 Boss ModeTag
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FGameplayTag CurrentModeTag;

	// BossAbilityExecutorComponent의 현재 실행 상태
	UPROPERTY(VisibleAnywhere, Category = "Output")
	ENSBossAbilityExecutionState ExecutionState = ENSBossAbilityExecutionState::Idle;

	// 현재 또는 마지막으로 실행한 AttackId
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FName CurrentAttackId = NAME_None;
};

USTRUCT(meta = (DisplayName = "Boss Combat", Category = "NeoSanctum|Boss"))
struct NEOSANCTUM_API FNSSTEvaluator_BossCombat : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNSSTEvaluator_BossCombatInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	// StateTree Context에서 BossAIController를 찾는 함수
	ANSBossAIController* ResolveBossController(FStateTreeExecutionContext& Context) const;

	// StateTree Context와 BossAIController에서 Boss Pawn을 찾는 함수
	APawn* ResolveBossPawn(FStateTreeExecutionContext& Context, ANSBossAIController* BossController) const;

	// 현재 Boss 전투 상태를 InstanceData에 갱신하는 함수
	void UpdateInstanceData(FStateTreeExecutionContext& Context) const;
};

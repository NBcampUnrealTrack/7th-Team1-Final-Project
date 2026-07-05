// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "NSSTTask_ExecuteBossAbility.generated.h"

class UNSBossAbilityExecutorComponent;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.05
 * 
 * 클래스 개요 : Boss StateTree에서 BossAbilityExecutorComponent에 공격 실행을 요청하고 실행 결과를 StateTree 상태로 반환하는 Task
*/
USTRUCT()
struct FNSSTTask_ExecuteBossAbilityInstanceData
{
	GENERATED_BODY()

	// 지정 공격만 실행할 때 사용하는 AttackId. None이면 조건 기반 자동 선택 사용
	UPROPERTY(EditAnywhere, Category = "Input")
	FName FixedAttackId = NAME_None;

	// StateTree 상태가 중간에 종료될 때 실행 중인 Ability를 취소할지 여부
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bCancelAbilityOnExit = true;

	// 이번 Task가 요청한 ExecutorComponent
	UPROPERTY(Transient)
	TWeakObjectPtr<UNSBossAbilityExecutorComponent> CachedExecutor;
};

USTRUCT(meta = (DisplayName = "Execute Boss Ability", Category = "NeoSanctum|Boss"))
struct NEOSANCTUM_API FNSSTTask_ExecuteBossAbility : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNSSTTask_ExecuteBossAbilityInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	// StateTree Context에서 BossAbilityExecutorComponent를 찾는 함수
	UNSBossAbilityExecutorComponent* ResolveExecutor(FStateTreeExecutionContext& Context) const;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "NSSTTask_TitanWalkerMove.generated.h"

class ANSBossAIController;
class UNSTitanWalkerMoveComponent;

USTRUCT()
struct FNSSTTask_TitanWalkerMoveInstanceData
{
	GENERATED_BODY()

	// StateTree Evaluator에서 전달받는 이동 기준 타깃
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 이번 Task가 구동하는 TitanWalker 이동 컴포넌트
	UPROPERTY(Transient)
	TWeakObjectPtr<UNSTitanWalkerMoveComponent> CachedMoveComponent;

	// 상태 종료 시 이동을 정지할지 여부
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bStopMoveOnExit = true;
};

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.06
 *
 * 클래스 개요 : TitanWalker StateTree의 MobileMode에서 이동 컴포넌트를 구동하는 Task
*/
USTRUCT(meta = (DisplayName = "Titan Walker Move", Category = "NeoSanctum|Boss"))
struct NEOSANCTUM_API FNSSTTask_TitanWalkerMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNSSTTask_TitanWalkerMoveInstanceData;

	FNSSTTask_TitanWalkerMove();

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
	// StateTree Context에서 TitanWalker 이동 컴포넌트를 찾는 함수
	UNSTitanWalkerMoveComponent* ResolveMoveComponent(FStateTreeExecutionContext& Context) const;

	// StateTree Context에서 BossAIController를 찾는 함수
	ANSBossAIController* ResolveBossController(FStateTreeExecutionContext& Context) const;

	// InstanceData 또는 Controller에서 이동 기준 타깃을 찾는 함수
	AActor* ResolveTargetActor(
		FStateTreeExecutionContext& Context,
		const FInstanceDataType& InstanceData) const;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_FindPatrolLocation.generated.h"

/**
 * 몬스터 주변의 무작위 NavMesh 도달 가능 좌표를 계산하여 블랙보드의 Vector 변수에 주입하는 태스크
 */
UCLASS()
class NEOSANCTUM_API UNSBTTask_FindPatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNSBTTask_FindPatrolLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// 정찰 반경
	UPROPERTY(EditAnywhere, Category = "AI|Config")
	float PatrolRadius = 800.0f;

	// 값을 저장할 블랙보드 키 선택 인터페이스
	UPROPERTY(EditAnywhere, Category = "AI|Config")
	FBlackboardKeySelector PatrolLocationKey;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_SetMeleeEQSRefresh.generated.h"

class UBehaviorTree;

UCLASS()
class NEOSANCTUM_API UNSBTTask_SetMeleeEQSRefresh : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// Task 이름과 Blackboard 키 필터를 초기화하는 함수
	UNSBTTask_SetMeleeEQSRefresh();

	// Behavior Tree Blackboard에 선택한 키를 연결하는 함수
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	// Blackboard의 EQS 재탐색 요청 상태를 지정된 값으로 변경하는 함수
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

protected:
	// EQS 재탐색 상태를 저장할 Blackboard Bool 키
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	FBlackboardKeySelector NeedsRefreshKey;

	// Task 실행 시 Blackboard에 기록할 재탐색 상태
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	bool bNeedsRefresh = true;
};

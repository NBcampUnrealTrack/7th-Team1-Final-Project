// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "NSBTTask_FlyMoveTo.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTTask_FlyMoveTo : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTTask_FlyMoveTo();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	// 이동 목표(Vector)를 읽어올 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "FlyMove")
	FBlackboardKeySelector MoveTargetKey;

	// 수평 도착 판정 반경
	UPROPERTY(EditAnywhere, Category = "FlyMove", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 100.f;

	// 도착 시 Succeeded로 끝낼지. false면 계속 추적(추격/카이팅에 사용)
	UPROPERTY(EditAnywhere, Category = "FlyMove")
	bool bFinishOnArrival = false;

};

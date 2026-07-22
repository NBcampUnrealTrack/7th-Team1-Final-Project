// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_FlyFaceTarget.generated.h"

UCLASS()
class NEOSANCTUM_API UNSBTTask_FlyFaceTarget : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UNSBTTask_FlyFaceTarget();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	// 타겟 액터를 읽어올 블랙보드 키 (Object)
	UPROPERTY(EditAnywhere, Category = "FaceTarget")
	FBlackboardKeySelector TargetActorKey;

	// 정렬 판정 각도 오차 허용치(도)
	UPROPERTY(EditAnywhere, Category = "FaceTarget", meta = (ClampMin = "0.0"))
	float AlignThresholdDegrees = 12.f;

	// 정렬 대기 안전 타임아웃(초)
	UPROPERTY(EditAnywhere, Category = "FaceTarget", meta = (ClampMin = "0.0"))
	float MaxTurnInTime = 1.f;

private:
	float ElapsedTime = 0.f;
};
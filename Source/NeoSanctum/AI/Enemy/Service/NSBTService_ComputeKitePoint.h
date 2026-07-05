// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "NSBTService_ComputeKitePoint.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTService_ComputeKitePoint : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTService_ComputeKitePoint();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	// 타겟과 유지할 거리(링 반경)
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0.0"))
	float DesiredDistance = 800.f;

	// 현재 방위에서 옆으로 둘 각도(선회/회피 강도, 도)
	UPROPERTY(EditAnywhere, Category = "Kite")
	float StrafeOffsetAngle = 30.f;

	// 선회 방향 (+1 시계 / -1 반시계)
	UPROPERTY(EditAnywhere, Category = "Kite")
	float StrafeDirection = 1.f;

	// 타겟 액터를 읽어올 블랙보드 키 (Object). 출력 MoveTarget은 부모의 BlackboardKey(Vector) 사용
	UPROPERTY(EditAnywhere, Category = "Kite")
	FBlackboardKeySelector TargetActorKey;
};

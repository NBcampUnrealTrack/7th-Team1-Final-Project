// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "NSBTTask_DroneMoveTo.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTTask_DroneMoveTo : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTTask_DroneMoveTo();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector MoveTargetKey;
	
	UPROPERTY(EditAnywhere, Category="DroneAI")
	float MoveSpeedOverride = 0.f;
	UPROPERTY(EditAnywhere, Category="DroneAI")
	bool bFinishOnArrival = false;
	
	UPROPERTY(EditAnywhere, Category="DroneAI")
	float AcceptanceRadius = 120.f;
};

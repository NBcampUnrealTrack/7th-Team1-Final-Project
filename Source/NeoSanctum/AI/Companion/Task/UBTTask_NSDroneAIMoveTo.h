// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "UBTTask_NSDroneAIMoveTo.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UUBTTask_NSDroneAIMoveTo : public UBTTask_MoveTo
{
	GENERATED_BODY()
	
public:
	UUBTTask_NSDroneAIMoveTo();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
	float HoverHeight = 300.f;
	
};

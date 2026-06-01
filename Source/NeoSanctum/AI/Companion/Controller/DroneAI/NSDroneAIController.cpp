// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAIController.h"

ANSDroneAIController::ANSDroneAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	
}

void ANSDroneAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("RunBehaviorTree"));
		RunBehaviorTree(BehaviorTreeAsset);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("NO RunBehaviorTree"));
}

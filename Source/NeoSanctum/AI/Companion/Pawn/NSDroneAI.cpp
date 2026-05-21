// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAI.h"
#include "Components/StaticMeshComponent.h"

ANSDroneAI::ANSDroneAI()
{
	PrimaryActorTick.bCanEverTick = false;
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	
	
}

void ANSDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	DroneAIController = Cast<ANSDroneAIController>(GetController());
	
	if (DroneAIController)
	{
		DroneAIBBComponent = DroneAIController->GetBlackboardComponent();
	}
}




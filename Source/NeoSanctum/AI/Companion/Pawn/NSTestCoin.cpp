// Copyright 2026 One Team. All rights reserved.


#include "NSTestCoin.h"
#include "EngineUtils.h"
#include "NeoSanctum/AI/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Pawn/NSDroneAI.h"


ANSTestCoin::ANSTestCoin()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANSTestCoin::RegisterPriorityActor()
{
	for (const TWeakObjectPtr<ANSDroneAIController>& DroneAIController : CashecDroneAIControllers)
	{
		if (!DroneAIController.IsValid()) continue;
		
		ANSDroneAIController* DroneAIC = DroneAIController.Get();
		if (!IsValid(DroneAIC)) continue;
		
		DroneAIC->SetPriorityActor(this);
	}
}


void ANSTestCoin::BeginPlay()
{
	Super::BeginPlay();
	RegisterPriorityActor();
}

void ANSTestCoin::Destroyed()
{
	for (const TWeakObjectPtr<ANSDroneAIController>& DroneAIController : CashecDroneAIControllers)
	{
		if (!DroneAIController.IsValid()) continue;
		
		ANSDroneAIController* DroneAIC = DroneAIController.Get();
		if (!IsValid(DroneAIC)) continue;
		
		DroneAIC->RemoveCoinActor(this);
	}
	
	Super::Destroyed();
}



// Copyright 2026 One Team. All rights reserved.


#include "ANSTestCoin.h"
#include "EngineUtils.h"
#include "NeoSanctum/AI/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Pawn/NSDroneAI.h"


AANSTestCoin::AANSTestCoin()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AANSTestCoin::RegisterPriorityActor()
{
	for (TActorIterator<ANSDroneAI> It(GetWorld()); It; ++It)
	{
		ANSDroneAIController* DroneAIController = Cast<ANSDroneAIController>((*It)->GetController());
		if (DroneAIController)
		{
			DroneAIController->SetPriorityActor(this);
		}
	}
}


void AANSTestCoin::BeginPlay()
{
	Super::BeginPlay();
	RegisterPriorityActor();
}



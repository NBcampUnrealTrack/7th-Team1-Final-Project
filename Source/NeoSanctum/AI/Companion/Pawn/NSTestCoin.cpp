// Copyright 2026 One Team. All rights reserved.


#include "NSTestCoin.h"
#include "EngineUtils.h"
#include "NeoSanctum/AI/Controller/DroneAI/NSDroneAIController.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"


ANSTestCoin::ANSTestCoin()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CoinPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("CoinPerception"));
	
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("StimuliSource");
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(TSubclassOf<UAISense_Sight>(UAISense_Sight::StaticClass()));
}

void ANSTestCoin::RegisterPriorityActor()
{
	for (const TWeakObjectPtr<ANSDroneAIController>& DroneAIController : CacheDroneAIControllers)
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
	for (TActorIterator<ANSDroneAIController> It(GetWorld()); It; ++It)
	{
		ANSDroneAIController* DroneAIC = *It;
		if (DroneAIC)
		{
			CacheDroneAIControllers.Add(DroneAIC);
		}
	}
}

void ANSTestCoin::Destroyed()
{
	Super::Destroyed();
}



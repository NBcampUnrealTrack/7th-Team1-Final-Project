// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAIController.h"
#include "EngineUtils.h"


const FName ANSDroneAIController::OwningPlayer = TEXT("OwningPlayer");
const FName ANSDroneAIController::PriorityActor = TEXT("PriorityActor");
const FName ANSDroneAIController::FindCoinActor = TEXT("FindCoinActor");

ANSDroneAIController::ANSDroneAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	DroneAIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	DroneAISightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Config_Sight"));
	
	DroneAISightConfig->SightRadius = 1500.0f; // 시야 범위
	DroneAISightConfig->LoseSightRadius = 2000.f; // 시야 상실 범위
	DroneAISightConfig->PeripheralVisionAngleDegrees = 60.f; // 주변 시야 각도
	
	DroneAISightConfig->DetectionByAffiliation.bDetectEnemies = true;
	DroneAISightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	DroneAISightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	DroneAIPerceptionComponent->ConfigureSense(*DroneAISightConfig);
	DroneAIPerceptionComponent->SetDominantSense(DroneAISightConfig->GetSenseImplementation());
}

void ANSDroneAIController::BeginPlay()
{
	Super::BeginPlay();
	
}

void ANSDroneAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (DroneAIBehaviorTree)
	{
		UBlackboardComponent* BBComp = Blackboard;
		UseBlackboard(DroneAIBehaviorTree->BlackboardAsset, BBComp);
		RunBehaviorTree(DroneAIBehaviorTree);
		
		DroneAIBBComponent = Blackboard;
	}
	
	FTimerHandle OwnerTimerHandle;
	GetWorldTimerManager().SetTimer(OwnerTimerHandle, this, &ANSDroneAIController::SetOwnerPlayer,0.5f, false);
}

void ANSDroneAIController::SetPriorityActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}
	
	if (DroneAIBBComponent)
	{
		DroneAIBBComponent->SetValueAsObject(PriorityActor, InActor);
	}
}

void ANSDroneAIController::SetOwnerPlayer()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if ((*It)->ActorHasTag("Player"))
		{
			if (DroneAIBBComponent)
			{
				DroneAIBBComponent->SetValueAsObject(OwningPlayer, *It);
				break;
			}
		}
	}
}

UBlackboardComponent* ANSDroneAIController::GetBlackboardComponent() const
{
	return Blackboard;
}



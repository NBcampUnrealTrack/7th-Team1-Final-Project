// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAIController.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/AI/Companion/Pawn/NSTestCoin.h"

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
	
	SetOwnerPlayer();
	DroneAIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ANSDroneAIController::OnUpdatePerception);
	
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

void ANSDroneAIController::OnUpdatePerception(AActor* InActor, FAIStimulus Stimulus)
{
	ANSTestCoin* Coin = Cast<ANSTestCoin>(InActor);
	if (!IsValid(Coin)) return;
	if (!IsValid(DroneAIBBComponent)) return;
	if (!Coin->ActorHasTag("Coin")) return;
	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>()) return;
		
	if (Stimulus.WasSuccessfullySensed())
	{
		DroneAIBBComponent->SetValueAsObject(FindCoinActor, Coin);
	}
}

UBlackboardComponent* ANSDroneAIController::GetBlackboardComponent() const
{
	return Blackboard;
}



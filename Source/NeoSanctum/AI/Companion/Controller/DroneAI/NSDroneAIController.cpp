// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAIController.h"
#include "EngineUtils.h"
#include "BehaviorTree/BehaviorTree.h"
#include "NeoSanctum/AI/Companion/Pawn/NSBasicDroneAI.h"
#include "NeoSanctum/AI/Companion/Pawn/NSTestCoin.h"

const FName ANSDroneAIController::OwningPlayer = TEXT("OwningPlayer");
const FName ANSDroneAIController::PriorityActor = TEXT("PriorityActor");
const FName ANSDroneAIController::FindCoinActor = TEXT("FindCoinActor");

ANSDroneAIController::ANSDroneAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	DroneAIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	DroneAISightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Config_Sight"));
	
	DroneAISightConfig->SightRadius = 500.0f; // 시야 범위
	DroneAISightConfig->LoseSightRadius = 600.f; // 시야 상실 범위
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

	if (!IsValid(DroneAIBBComponent)) return;
	
	ANSBasicDroneAI* MyPawn = Cast<ANSBasicDroneAI>(InPawn);
	if (!IsValid(MyPawn)) return;
	
	if (DroneAIBBComponent->GetValueAsObject(OwningPlayer) != nullptr)
	{
		DroneAIBBComponent->ClearValue(OwningPlayer);
	}
	
	DroneAIBBComponent->SetValueAsObject(OwningPlayer, MyPawn->GetOwnerPlayer());
	
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
	if (!IsValid(DroneAIBBComponent)) return;
	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>()) return;
	
	ANSTestCoin* Coin = Cast<ANSTestCoin>(InActor);
	if (!IsValid(Coin) || !Coin->ActorHasTag("Coin")) return;
	
	if (Stimulus.WasSuccessfullySensed())
	{
		UpdateTargetCoin(Coin);
	}
	else
	{
		RemoveTargetCoin(Coin);
	}
}

void ANSDroneAIController::UpdateTargetCoin(ANSTestCoin* TargetCoin)
{
	if (!IsValid(DroneAIBBComponent)) return;
	
	UObject* TargetActor = DroneAIBBComponent->GetValueAsObject(FindCoinActor);
	if (TargetActor != nullptr) return;
	
	DroneAIBBComponent->SetValueAsObject(FindCoinActor, TargetCoin);
}

void ANSDroneAIController::RemoveTargetCoin(ANSTestCoin* TargetCoin)
{
	if (!IsValid(DroneAIBBComponent)) return;
	
	UObject* TargetActor = DroneAIBBComponent->GetValueAsObject(FindCoinActor);
	if (TargetActor == nullptr) return;
	
	if (TargetActor == TargetCoin)
	{
		DroneAIBBComponent->ClearValue(FindCoinActor);
	}
	
}

UBlackboardComponent* ANSDroneAIController::GetBlackboardComponent() const
{
	return DroneAIBBComponent;
}

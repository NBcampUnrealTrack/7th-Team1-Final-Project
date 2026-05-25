// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAIController.h"
#include "EngineUtils.h"
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
	ANSTestCoin* Coin = Cast<ANSTestCoin>(InActor);
	if (!IsValid(Coin)) return;
	if (!Coin->ActorHasTag("Coin")) return;
	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>()) return;
	UE_LOG(LogTemp,Warning,TEXT("Call UpdatePerception"));
	
	if (Stimulus.WasSuccessfullySensed())
	{
		
		UE_LOG(LogTemp,Warning,TEXT("Call WasSuccessfullySensed"));
		Coins.Add(Coin);
	}
	else
	{
		Coins.Remove(Coin);
	}
	
	UpdateTargetCoin();
}

void ANSDroneAIController::UpdateTargetCoin()
{
	if (Coins.IsEmpty()) return;
	if (!IsValid(DroneAIBBComponent)) return;
	for (ANSTestCoin* Coin : Coins)
	{
		if (IsValid(Coin))
		{
			DroneAIBBComponent->SetValueAsObject(FindCoinActor, Coin);
		}
	}
}

void ANSDroneAIController::RemoveTargetCoin(const ANSTestCoin* TargetCoin)
{
	for (ANSTestCoin* Coin : Coins)
	{
		if (TargetCoin == Coin)
		{
			Coins.Remove(Coin);
			UpdateTargetCoin();
		}
	}
}

UBlackboardComponent* ANSDroneAIController::GetBlackboardComponent() const
{
	return Blackboard;
}

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
	
	FTimerHandle OwnerTimerHandle;
	GetWorldTimerManager().SetTimer(OwnerTimerHandle, this, &ANSDroneAIController::SetOwnerPlayer,0.5f, false);
}

void ANSDroneAIController::SetPriorityActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}
	
	ANSTestCoin* CoinActor = Cast<ANSTestCoin>(InActor);
	
	if (DroneAIBBComponent)
	{
		DroneAIBBComponent->SetValueAsObject(PriorityActor, CoinActor);
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
	
	UE_LOG(LogTemp, Warning, TEXT("InActor: %s"), *InActor->GetClass()->GetName());
	
	ANSTestCoin* Coin = Cast<ANSTestCoin>(InActor);
	UE_LOG(LogTemp, Warning, TEXT("Cast Result: %s"), 
		IsValid(Coin) ? TEXT("Success") : TEXT("Failed"));
	
	
	UE_LOG(LogTemp, Warning, TEXT("Stimulus Type: %d"), Stimulus.Type.Index);
	UE_LOG(LogTemp, Warning, TEXT("Sight ID: %d"), UAISense::GetSenseID<UAISense_Sight>().Index);
	
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		UE_LOG(LogTemp, Warning, TEXT("Call bCanSeeCoin"));
		bool bCanSeeCoin = Stimulus.WasSuccessfullySensed();
		if (bCanSeeCoin)
		{
			if (DroneAIBBComponent)
			{
				DroneAIBBComponent->SetValueAsBool(FindCoinActor, bCanSeeCoin);
				SetPriorityActor(Coin);
			}
		}
		else
		{
			if (DroneAIBBComponent)
			{
				DroneAIBBComponent->SetValueAsBool(FindCoinActor, bCanSeeCoin);
			}
		}
	}
	
}

UBlackboardComponent* ANSDroneAIController::GetBlackboardComponent() const
{
	return Blackboard;
}



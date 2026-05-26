// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NSDroneAIController.generated.h"

class ANSTestCoin;

UENUM(BlueprintType)
enum EDroneState
{
	Idle = 0,
	Searching = 1,
	Chasing  = 2
};

UCLASS()
class NEOSANCTUM_API ANSDroneAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSDroneAIController();
	
	UBlackboardComponent* GetBlackboardComponent() const;
	
	UFUNCTION()
	void SetOwnerPlayer();
	
	UFUNCTION()
	void OnUpdatePerception(AActor* InActor, FAIStimulus Stimulus);
	
	void UpdateTargetCoin();
	void RemoveTargetCoin(const ANSTestCoin* TargetCoin);
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "DroneAI|Behavior")
	UBehaviorTree* DroneAIBehaviorTree;
	
	UPROPERTY(BlueprintReadWrite, Category= "DroneAI|Behavior")
	UBehaviorTreeComponent* DroneAIBTComponent;
	
	UPROPERTY(BlueprintReadWrite, Category= "DroneAI|Behavior")
	UBlackboardComponent* DroneAIBBComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "DroneAI|Perception")
	TObjectPtr<UAIPerceptionComponent> DroneAIPerceptionComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "DroneAI|Perception")
	TObjectPtr<UAISenseConfig_Sight> DroneAISightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "DroneAI|Perception")
	TArray<ANSTestCoin*> Coins;
private:
	// 블랙보드 키 변수
	static const FName OwningPlayer;
	static const FName PriorityActor;
	static const FName FindCoinActor;
};

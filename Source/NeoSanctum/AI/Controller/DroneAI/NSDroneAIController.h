// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NSDroneAIController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSDroneAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSDroneAIController();

	static const FName OwningPlayer;
	static const FName PriorityActor;
	
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	UFUNCTION()
	void SetPriorityActor();
	
	UFUNCTION()
	void SetOwnerPlayer();
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "DroneAI|Behavior")
	UBehaviorTree* DroneAIBehaviorTree;
	
	UPROPERTY(BlueprintReadWrite, Category= "DroneAI|Behavior")
	UBehaviorTreeComponent* DroneAIBTComponent;
	
	UPROPERTY(BlueprintReadWrite, Category= "DroneAI|Behavior")
	UBlackboardComponent* DroneAIBBComponent;
	
	UBlackboardComponent* GetBlackboardComponent() const;
};

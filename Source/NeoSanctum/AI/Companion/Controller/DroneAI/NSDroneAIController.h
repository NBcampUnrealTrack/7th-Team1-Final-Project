// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NSDroneAIController.generated.h"

class UBehaviorTree;

UCLASS()
class NEOSANCTUM_API ANSDroneAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSDroneAIController();
	
	UFUNCTION(BlueprintCallable, Category="DroneAI")
	void SetOwnerPlayer (APawn* InOwnerPlayer) {OwnerPlayer = InOwnerPlayer;}

	UFUNCTION(BlueprintCallable, Category="DroneAI")
	APawn* GetOwnerPlayer() {return OwnerPlayer;}
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(EditDefaultsOnly, Category="DroneAI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
	
private:
	UPROPERTY()
	APawn* OwnerPlayer = nullptr;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSTestCoin.generated.h"

class ANSDroneAIController;

UCLASS()
class NEOSANCTUM_API ANSTestCoin : public AActor
{
	GENERATED_BODY()

public:
	ANSTestCoin();

	void RegisterPriorityActor();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void Destroyed() override;
	
private:
	UPROPERTY(VisibleAnywhere)
	TArray<TWeakObjectPtr<ANSDroneAIController>> CashecDroneAIControllers;

};

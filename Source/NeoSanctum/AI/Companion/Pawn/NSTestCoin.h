// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Perception/AIPerceptionComponent.h"
#include "NSTestCoin.generated.h"

class ANSDroneAIController;
class UAIPerceptionStimuliSourceComponent;

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
	UPROPERTY(VisibleAnywhere, Category="Coin|CacheData")
	TArray<TWeakObjectPtr<ANSDroneAIController>> CacheDroneAIControllers;
	
	UPROPERTY(VisibleAnywhere, Category="Coin|Perception")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;
	
	UPROPERTY(VisibleAnywhere, Category="Coin|Perception")
	TObjectPtr<UAIPerceptionComponent> CoinPerception;
	
};

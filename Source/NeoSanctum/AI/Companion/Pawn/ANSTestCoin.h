// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ANSTestCoin.generated.h"

UCLASS()
class NEOSANCTUM_API AANSTestCoin : public AActor
{
	GENERATED_BODY()

public:
	AANSTestCoin();

	void RegisterPriorityActor();
	
protected:
	virtual void BeginPlay() override;

};

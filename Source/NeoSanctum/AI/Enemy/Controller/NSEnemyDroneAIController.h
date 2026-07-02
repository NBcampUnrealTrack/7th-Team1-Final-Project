// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NSEnemyDroneAIController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSEnemyDroneAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSEnemyDroneAIController();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};

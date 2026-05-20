// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NSDroneAIController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSDroneAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSDroneAIController();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};

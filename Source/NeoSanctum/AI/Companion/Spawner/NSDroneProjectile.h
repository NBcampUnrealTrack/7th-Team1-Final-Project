// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSDroneProjectile.generated.h"

UCLASS()
class NEOSANCTUM_API ANSDroneProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANSDroneProjectile();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};

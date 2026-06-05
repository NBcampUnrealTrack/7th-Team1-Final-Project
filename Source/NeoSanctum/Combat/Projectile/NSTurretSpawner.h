// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSThrowProjectileBase.h"
#include "NSTurretSpawner.generated.h"

UCLASS()
class NEOSANCTUM_API ANSTurretSpawner : public ANSThrowProjectileBase
{
	GENERATED_BODY()

public:
	ANSTurretSpawner();
	
protected:
	virtual void BeginPlay() override;
	
private:
	UFUNCTION()
	void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "TurretSpawner")
	TSubclassOf<AActor> TurretClass;
};

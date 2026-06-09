// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSThrowProjectileBase.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_ThrowProjectile.h"
#include "NSTurretSpawner.generated.h"

class ANSTurret;

UCLASS()
class NEOSANCTUM_API ANSTurretSpawner : public ANSThrowProjectileBase
{
	GENERATED_BODY()

public:
	ANSTurretSpawner();

	void InitializeTurretSpawner(const FNSTurretSpawnerTypeConfig& InConfig);
	
protected:
	virtual void BeginPlay() override;
	
private:
	UFUNCTION()
	void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	
private:
	bool IsSpawnableSurface(const FHitResult& ImpactResult) const;
	void SpawnTurret(const FHitResult& ImpactResult);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "TurretSpawner")
	TSubclassOf<ANSTurret> TurretClass;
	
	UPROPERTY(VisibleAnywhere, Category = "TurretSpawner")
	float MaxSpawnableAngle = 30.0f;

	FNSTurretConfig TurretConfig;
	
	bool bSpawned = false;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSTurret.generated.h"

class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSTurret : public AActor
{
	GENERATED_BODY()

public:
	ANSTurret();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UStaticMeshComponent> BaseMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USceneComponent> HeadPivotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UStaticMeshComponent> HeadMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USphereComponent> DetectionSphereComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Detection")
	float DetectionRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSBombMissile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class NEOSANCTUM_API ANSBombMissile : public AActor
{
	GENERATED_BODY()

public:
	ANSBombMissile();

	// 현재 위치에서 수직 낙하를 시작. LandingLocation 도달 시 이동을 정지하고 대기
	void InitDrop(const FVector& LandingLocation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombMissile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombMissile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombMissile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BombMissile|Drop", meta = (ClampMin = "0.0"))
	float DropSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BombMissile|Drop", meta = (ClampMin = "0.0"))
	float DropGravityScale = 1.f;

private:
	FVector TargetLandingLocation = FVector::ZeroVector;
	bool bIsDropping = false;
};
// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSRangerProjectile.generated.h"

/*
 * Ranger 투사체 기본 Actor
 * 이동과 충돌은 Projectile이 담당하고, GA는 스폰까지만 담당
 */

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSRangerProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANSRangerProjectile();

protected:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMeshComponent;			
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float LifeSeconds = 5.0f;

public:
	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& HitResult
	);
};

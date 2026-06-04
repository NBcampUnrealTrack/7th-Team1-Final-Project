// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSThrowProjectileBase.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS(Abstract)
class NEOSANCTUM_API ANSThrowProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	ANSThrowProjectileBase();

public:
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void InitializeThrowActor(
		APawn* InOwningPawn,
		AController* InOwningController,
		const FVector& ThrowDirection
	);

protected:
	APawn* GetOwningPawn() const { return OwningPawn; }
	AController* GetOwningController() const { return OwningController; }
	USphereComponent* GetCollisionComponent() const { return CollisionComponent; }
	UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
	UProjectileMovementComponent* GetProjectileMovementComponent() const { return ProjectileMovementComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Projectile|Owner")
	TObjectPtr<APawn> OwningPawn;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Projectile|Owner")
	TObjectPtr<AController> OwningController;
};

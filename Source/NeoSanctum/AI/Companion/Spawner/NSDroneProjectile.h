// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "NSDroneProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSDroneProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANSDroneProjectile();

	void InitProjectile(
		const FVector& Direction, APawn* InInstigator,
		const FGameplayEffectSpecHandle& InDamageSpec, float ProjectileSpeed);
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnHit(
		UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION()
	void OnRep_InitialSpeed();
	
	void ApplyVelocity();
	
protected:
	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TObjectPtr<USphereComponent> CollisionComp;
	
	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;
	
	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComp;
	
	FGameplayEffectSpecHandle DamageSpecHandle;
	
	UPROPERTY(ReplicatedUsing=OnRep_InitialSpeed)
	float InitialSpeed = 0.f;
	
};

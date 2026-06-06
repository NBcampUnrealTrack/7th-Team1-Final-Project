// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSRangerProjectile.generated.h"


class UGameplayEffect;
class UAbilitySystemComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Ranger 투사체 기본 Actor
 * 이동과 충돌은 Projectile이 담당하고, GA는 스폰까지만 담당
 */
UCLASS()
class NEOSANCTUM_API ANSRangerProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANSRangerProjectile();
	
	void InitializeProjectile(
		UAbilitySystemComponent* InSourceASC,
		TSubclassOf<UGameplayEffect> InSplashDamageEffectClass,
		float InSplashDamageEffectLevel
	);
	
	void LaunchProjectile(const FVector& LaunchDirection);

protected:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMeshComponent;			
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float LifeSeconds = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Splash")
	float ExplosionRadius = 300.0f;
	
protected:
	// Debug
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Debug")
	bool bDrawDebugExplosion = false;

private:
	void IgnoreSourceActorCollision();
	
	void FindSplashTargetActors(const FVector& ExplosionLocation, TArray<AActor*>& OutTargetActors) const;
	
	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& HitResult
	);
	
	void ExecuteImpactCue(const FHitResult& HitResult);
	
	void ApplySplashDamage(const FVector& ExplosionLocation, const TArray<AActor*>& TargetActors) const;
	
private:
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC;
	
	UPROPERTY()
	TSubclassOf<UGameplayEffect> SplashDamageEffectClass;
	
	float SplashDamageEffectLevel = 1.0f;
};

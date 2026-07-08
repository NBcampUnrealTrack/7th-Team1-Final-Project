// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "NSHomingMissile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;


UCLASS()
class NEOSANCTUM_API ANSHomingMissile : public AActor
{
	GENERATED_BODY()

public:
    ANSHomingMissile();

    void InitializeMissile(
        UAbilitySystemComponent* InSourceASC,
        TSubclassOf<UGameplayEffect> InDamageEffectClass,
        float InExplosionDamage,
        float InExplosionRadius,
        USceneComponent* InHomingTarget,
        const TArray<AActor*>& InIgnoredActors);

    void LaunchMissile(const FVector& LaunchDirection);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Speed", meta = (ClampMin = "1.0"))
    float InitialSpeed = 800.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Speed", meta = (ClampMin = "1.0"))
    float MaxSpeed = 3500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Speed", meta = (ClampMin = "0.0"))
    float Acceleration = 2500.0f;

    // 낮을수록 선회가 느려져 대쉬 회피가 쉬워짐
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Homing", meta = (ClampMin = "0.0"))
    float HomingAccelerationMagnitude = 4000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile", meta = (ClampMin = "0.01"))
    float LifeSeconds = 8.0f;

    // Row->AreaData.Radius가 0으로 들어오면 이 값을 그대로 사용(폭발 반경 BP 기본값)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Explosion", meta = (ClampMin = "0.0"))
    float ExplosionRadius = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Explosion")
    bool bExcludeOccludedSplashTargets = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Explosion", meta = (ClampMin = "0.0"))
    float SplashOcclusionTraceStartOffset = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Cue")
    FGameplayTag ImpactCueTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Debug")
    bool bDrawDebugExplosion = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|Debug")
    bool bDrawDebugSplashOcclusion = false;

private:
    void IgnoreCollisionForActors(const TArray<AActor*>& ActorsToIgnore);

    void FindSplashTargetActors(const FVector& ExplosionLocation, TArray<AActor*>& OutTargetActors) const;
    void FilterOccludedSplashTargets(const FVector& TraceStart, TArray<AActor*>& TargetActors) const;
    bool IsSplashTargetOccluded(const FVector& TraceStart, const AActor* TargetActor) const;

    UFUNCTION()
    void OnMissileHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& HitResult);

    void ExecuteImpactCue(const FHitResult& HitResult);
    void ApplyExplosionDamage(const FVector& ExplosionLocation, const TArray<AActor*>& TargetActors) const;

private:
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> SourceASC;

    UPROPERTY()
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    float ExplosionDamage = 0.0f;
    float CurrentSpeed = 0.0f;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
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
	
	// GA가 폭격 시점에 호출. 폭발 큐를 전 클라에 재생하고 파괴
	void Detonate();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombMissile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombMissile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BombMissile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 폭발 VFX GameplayCue (착탄 지점에서 재생)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BombMissile|Explosion")
	FGameplayTag ImpactCueTag;
	
	// 폭발 VFX가 재생될 시간을 확보한 뒤 파괴 (즉시 파괴 시 큐가 함께 사라짐)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BombMissile|Explosion", meta = (ClampMin = "0.0"))
	float ExplosionLingerSeconds = 3.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BombMissile|Drop", meta = (ClampMin = "0.0"))
	float DropSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BombMissile|Drop", meta = (ClampMin = "0.0"))
	float DropGravityScale = 1.f;

private:
	FVector TargetLandingLocation = FVector::ZeroVector;
	bool bIsDropping = false;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Detonate();

	void PlayImpactCue();
};
// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "NSTestCoin.generated.h"

class USceneComponent;
class USphereComponent;
class ANSDroneAIController;
class UAIPerceptionStimuliSourceComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

UCLASS()
class NEOSANCTUM_API ANSTestCoin : public AActor
{
	GENERATED_BODY()

public:
	ANSTestCoin();
	
	void CheckPlayerActor();
	
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
						AActor* OtherActor,
						UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex,
						bool bFromSweep,
						const FHitResult& SweepResult);

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category="Coin|CacheData")
	TArray<TWeakObjectPtr<ANSDroneAIController>> CacheDroneAIControllers;
	
	UPROPERTY(VisibleAnywhere, Category="Coin|Perception")
	TObjectPtr<USphereComponent> CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Coin|Perception")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Coin|Perception")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coin|Collision", meta=(AllowPrivateAccess=true))
	float MagneticRadius;
	
	FTimerHandle CheckPlayerTimerHandle;
};



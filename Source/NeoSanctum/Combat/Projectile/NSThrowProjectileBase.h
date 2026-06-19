// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
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

	void SetSetByCallerMagnitudes(const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes);
	void SetRuntimeStatMagnitudes(const TArray<FNSCombatStatMagnitude>& InRuntimeStatMagnitudes);

protected:
	APawn* GetOwningPawn() const { return OwningPawn; }
	AController* GetOwningController() const { return OwningController; }
	// 자식 투척물이 Turret 등에 전달할 payload
	const TArray<FNSSetByCallerMagnitude>& GetSetByCallerMagnitudes() const { return SetByCallerMagnitudes; }
	const TArray<FNSCombatStatMagnitude>& GetRuntimeStatMagnitudes() const { return RuntimeStatMagnitudes; }
	// 런타임 stat 값 조회
	bool TryGetRuntimeStatMagnitude(const FGameplayTag& CombatStatTag, float& OutMagnitude) const;
	// RuntimeStat float 값을 boolean 옵션처럼 사용할 때 0 초과면 true로 변환
	bool TryGetRuntimeStatBool(const FGameplayTag& CombatStatTag, bool& OutValue) const;
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

	// GE에 전달할 SetByCaller payload
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Projectile|SetByCaller")
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

	// 투척물 로직용 runtime payload
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Projectile|RuntimeStats")
	TArray<FNSCombatStatMagnitude> RuntimeStatMagnitudes;
};

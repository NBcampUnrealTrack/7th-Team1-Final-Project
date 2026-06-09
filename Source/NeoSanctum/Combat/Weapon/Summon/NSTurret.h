// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSTurret.generated.h"

class USphereComponent;
struct FNSTurretConfig;
class AController;
class APawn;

UCLASS()
class NEOSANCTUM_API ANSTurret : public AActor
{
	GENERATED_BODY()

public:
	ANSTurret();

	void InitializeTurret(const FNSTurretConfig& InConfig, APawn* InOwningPawn, AController* InOwningController);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnDetectionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnDetectionSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:
	bool IsValidTargetActor(const AActor* TargetActor) const;
	bool CanSeeTarget(const AActor* TargetActor) const;
	
	void InitializeTargets();
	void UpdateAutoTarget();
	
private:
	void RotateJointToTarget(float DeltaSeconds);
	void RotateHeadToTarget(float DeltaSeconds);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UStaticMeshComponent> BaseMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USceneComponent> JointPivotComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UStaticMeshComponent> JointMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USceneComponent> HeadPivotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UStaticMeshComponent> HeadMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<USphereComponent> DetectionSphereComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Detection")
	float DetectionRadius = 1500.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Detection")
	float TargetRefreshInterval = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Aim")
	float YawTurnSpeed = 360.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Aim")
	float PitchTurnSpeed = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");
	
protected:
	// Turret을 소환한 캐릭터 Pawn
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|Owner")
	TObjectPtr<APawn> OwningPawn;
	
	// Turret을 소환한 플레이어
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|Owner")
	TObjectPtr<AController> OwningController;
	
private:
	// 콜리전 안에 들어온 Actor 중에 Enemy TeamID를 가진 Actor 세트
	TSet<TWeakObjectPtr<AActor>> TargetSet;
	
	// 터렛의 최종 타겟이 된 액터(가장 가까운 Enemy ID를 가진 액터) 
	TWeakObjectPtr<AActor> AutoTarget;
	
	// 타겟을 재탐색하는 타이머
	FTimerHandle TargetRefreshTimerHandle;
};

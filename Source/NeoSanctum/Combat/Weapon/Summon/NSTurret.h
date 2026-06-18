// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSTurret.generated.h"

class UAbilitySystemComponent;
class UCapsuleComponent;
class UGameplayEffect;
class USphereComponent;
class UNSAbilitySystemComponent;
class UNSDissolveComponent;
class UNSTurretAttributeSet;
struct FNSTurretConfig;
struct FOnAttributeChangeData;
class AController;
class APawn;

UCLASS()
class NEOSANCTUM_API ANSTurret : public AActor,
                                 public IAbilitySystemInterface,
                                 public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ANSTurret();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// Turret 설정과 초기화 payload 전달
	void InitializeTurret(
		const FNSTurretConfig& InConfig,
		APawn* InOwningPawn,
		AController* InOwningController,
		const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes
	);

	float GetSpawnSurfaceOffset() const;
	APawn* GetOwningPawn() const { return OwningPawn; }
	AController* GetOwningController() const { return OwningController; }
	
public:
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(ETeamId::Player)); }

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

protected:
	// 터렛 비활성화(사망) 연출을 클라이언트에서 한 번 복제해야함.
	UFUNCTION()
	void OnRep_DeathPresentationStarted();
	
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

private:
	void InitializeAbilityActorInfo();
	void ApplyInitialAttributeEffect();
	// 초기화 GE에 SetByCaller payload 적용
	void ApplySetByCallerMagnitudes(FGameplayEffectSpecHandle& SpecHandle) const;

	void BindAttributeChangeDelegates();
	void HandleDetectionRangeChanged(const FOnAttributeChangeData& Data);
	void RefreshDetectionRange();

private:
	void InitializeTargets();
	void UpdateAutoTarget();
	void RestartTargetRefreshTimer();

private:
	void RotateJointToTarget(float DeltaSeconds);
	void RotateHeadToTarget(float DeltaSeconds);

private:
	void TryFire();
	bool CanFireToCurrentTarget() const;
	void FireHitscan();
	FTransform GetMuzzleTransform() const;
	
private:
	// Health Attribute가 0이 되는 순간 실행
	void HandleOutOfHealth();
	
	// 터렛 비활성화 진입
	void DeactivateTurret();
	void ApplyDeathState();
	void StartDeathPresentation();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> ASC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSTurretAttributeSet> AttributeSet;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UCapsuleComponent> HitCollisionComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UNSDissolveComponent> DissolveComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Detection")
	float TargetRefreshInterval = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Aim")
	float YawTurnSpeed = 360.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Aim")
	float PitchTurnSpeed = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Weapon")
	float FireAngleTolerance = 10.0f;
	
	// 탄 퍼짐 효과를 위한 최대 SpreadAngle
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Weapon")
	float MaxSpreadAngle = 10.0f;
	
	// 데미지 적용 GameplayEffect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Weapon")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
protected:
	// Attribute 초기화 GE
	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> InitialAttributeEffectClass;

	// Turret을 소환한 캐릭터 Pawn
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|Owner")
	TObjectPtr<APawn> OwningPawn;

	// Turret을 소환한 플레이어
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|Owner")
	TObjectPtr<AController> OwningController;

	// 초기 Attribute GE에 전달할 payload
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|SetByCaller")
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

private:
	// 콜리전 안에 들어온 Actor 중에 Enemy TeamID를 가진 Actor 세트
	TSet<TWeakObjectPtr<AActor>> TargetSet;

	// 터렛의 최종 타겟이 된 액터(가장 가까운 Enemy ID를 가진 액터) 
	TWeakObjectPtr<AActor> AutoTarget;

	// 타겟을 재탐색하는 타이머
	FTimerHandle TargetRefreshTimerHandle;

	bool bAbilityActorInfoInitialized = false;
	bool bInitialAttributeEffectApplied = false;
	bool bAttributeChangeDelegatesBound = false;

	float LastFireTime = 0.0f;
	bool bHasFired = false;
	
private:
	UPROPERTY(ReplicatedUsing = OnRep_DeathPresentationStarted)
	bool bDeathPresentationStarted = false;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "NeoSanctum/Core/Interface/NSPlayerAttackFeedbackSourceInterface.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSTurret.generated.h"

class UNSCombatStatComponent;
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
class UNSMeleeAttackReservationComponent;
class UNSHitReactionComponent;
class UNSDamageFlashComponent;

UCLASS()
class NEOSANCTUM_API ANSTurret : public AActor,
                                 public IAbilitySystemInterface,
                                 public IGenericTeamAgentInterface,
                                 public INSPlayerAttackFeedbackSourceInterface
{
	GENERATED_BODY()

public:
	ANSTurret();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ShouldTriggerPlayerAttackFeedback() const override { return false; }

	// Turret 설정과 초기화 payload 전달
	void InitializeTurret(
		const FNSTurretConfig& InConfig,
		APawn* InOwningPawn,
		AController* InOwningController,
		const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes,
		const TArray<FNSCombatStatMagnitude>& InRuntimeStatMagnitudes
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

	// 서버가 계산한 조준 회전값을 클라이언트 시각에 반영
	UFUNCTION()
	void OnRep_AimReplicationState();
	
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
	// 터렛 사격에 피격된 대상의 피해 가능 여부를 판정하는 함수
	bool CanDamageHitActor(const AActor* HitActor) const;
	bool CanSeeTarget(const AActor* TargetActor) const;

private:
	void InitializeAbilityActorInfo();
	void ApplyInitialAttributeEffect();

	// 설치 중 무적 GameplayEffect를 서버에서 적용.
	void ApplyDeploymentInvincibilityEffect();

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
	// 클라이언트에서 복제된 조준 회전값으로 보간
	void ApplyReplicatedAimRotation(float DeltaSeconds);

private:
	void TryFire();
	bool CanFireToCurrentTarget() const;
	void FireHitscan();
	// 터렛 자체 Attribute에는 크리티컬 스탯이 없으므로, 소환자의 현재 CritChance/CritDamage를
	// Damage GE Spec에 SetByCaller로 전달해 발사 시점 기준 크리티컬이 적용되게 함.
	void ApplyCritOverrideToSpec(FGameplayEffectSpecHandle& SpecHandle) const;
	void ReportFireNoise(const FVector& NoiseLocation);
	FTransform GetMuzzleTransform() const;
	FTransform GetTraceSocketTransform() const;
	
private:
	// Health Attribute가 0이 되는 순간 실행
	void HandleOutOfHealth();
	
	// 터렛 비활성화 진입
	void DeactivateTurret();
	void ApplyDeathState();
	void StartDeathPresentation();
	void StartLifetimeTimer();
	bool TryGetRuntimeStatMagnitude(const FGameplayTag& CombatStatTag, float& OutMagnitude) const;
	// 발사 판정마다 소환자 CombatStatComponent에서 FireRate를 라이브 조회. 실패 시 소환 시점 스냅샷으로 대체.
	float GetCurrentFireRate() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> ASC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSTurretAttributeSet> AttributeSet;

	UPROPERTY(Transient)
	TObjectPtr<UNSCombatStatComponent> OwningCombatStatComponent;

	FGameplayTag SourceAbilityTag;

	// 터렛 전용 GE가 만든 배율을 계산할 때 기준으로 쓸 소환 시점 공속.
	float BaseFireRateAtSpawn = 0.0f;

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

	// 실제 Health Damage를 받았을 때 월드 피격 리액션을 재생하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UNSHitReactionComponent> HitReactionComponent;

	// 피격 시 머티리얼 플래시를 재생하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Components")
	TObjectPtr<UNSDamageFlashComponent> DamageFlashComponent;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UNSMeleeAttackReservationComponent> MeleeAttackReservationComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Detection")
	float TargetRefreshInterval = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Aim")
	float YawTurnSpeed = 360.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret|Aim")
	float PitchTurnSpeed = 360.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Weapon")
	FName TraceSocketName = TEXT("Trace");

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

	// 터렛 설치가 끝날 때까지 무적 상태를 부여하는 GE.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Deployment")
	TSubclassOf<UGameplayEffect> DeploymentInvincibilityEffectClass;

	// Turret을 소환한 캐릭터 Pawn
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|Owner")
	TObjectPtr<APawn> OwningPawn;

	// Turret을 소환한 플레이어
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|Owner")
	TObjectPtr<AController> OwningController;

	// 초기 Attribute GE에 전달할 payload
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|SetByCaller")
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Turret|RuntimeStats")
	TArray<FNSCombatStatMagnitude> RuntimeStatMagnitudes;

private:
	// 콜리전 안에 들어온 Actor 중에 Enemy TeamID를 가진 Actor 세트
	TSet<TWeakObjectPtr<AActor>> TargetSet;

	// 터렛의 최종 타겟이 된 액터(가장 가까운 Enemy ID를 가진 액터) 
	TWeakObjectPtr<AActor> AutoTarget;

	// 타겟을 재탐색하는 타이머
	FTimerHandle TargetRefreshTimerHandle;

	FTimerHandle LifetimeTimerHandle;

	bool bAbilityActorInfoInitialized = false;
	bool bInitialAttributeEffectApplied = false;
	bool bAttributeChangeDelegatesBound = false;

	float LastFireTime = 0.0f;
	bool bHasFired = false;
	
private:
	UPROPERTY(ReplicatedUsing = OnRep_DeathPresentationStarted)
	bool bDeathPresentationStarted = false;

	// 클라이언트 조준 보간 Tick 활성화 여부
	UPROPERTY(ReplicatedUsing = OnRep_AimReplicationState)
	bool bReplicatedAimActive = false;

	// 서버 기준 Joint 회전값
	UPROPERTY(ReplicatedUsing = OnRep_AimReplicationState)
	FRotator ReplicatedJointRelativeRotation = FRotator::ZeroRotator;

	// 서버 기준 Head 회전값
	UPROPERTY(ReplicatedUsing = OnRep_AimReplicationState)
	FRotator ReplicatedHeadRelativeRotation = FRotator::ZeroRotator;

	// 클라이언트 보간 목표 회전값
	FRotator TargetJointRelativeRotation = FRotator::ZeroRotator;
	FRotator TargetHeadRelativeRotation = FRotator::ZeroRotator;
};

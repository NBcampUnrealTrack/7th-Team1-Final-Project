// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "GA_ThrowProjectile.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class UStaticMesh;
class UStaticMeshComponent;
class ANSThrowProjectileBase;
class ANSTurret;

UENUM(BlueprintType)
enum class EProjectileType : uint8
{
	Explosive,
	TurretSpawner
};

USTRUCT(BlueprintType)
struct FNSExplosiveTypeConfig
{
	GENERATED_BODY()

	// TODO : 폭발 Projectile 관련 설정
};

USTRUCT(BlueprintType)
struct FNSTurretConfig
{
	GENERATED_BODY()
	
	// 타겟 탐지 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Motion")
	float TargetRefreshInterval = 0.25f;
	
	// Yaw 회전속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Motion")
	float YawTurnSpeed = 360.0f;
	
	// Pitch 회전속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Motion")
	float PitchTurnSpeed = 360.0f;
	
	// 발사를 시작하게 되는 타겟과의 최소 각도 차이
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Combat")
	float FireAngleTolerance = 8.0f;
	
	// 데미지 적용 GameplayEffect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Weapon")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Attribute 초기화 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret|Attribute")
	TSubclassOf<UGameplayEffect> InitialAttributeEffectClass;
};

USTRUCT(BlueprintType)
struct FNSTurretSpawnerTypeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<ANSTurret> TurretClass;
	
	// 터렛 설정 구조체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FNSTurretConfig TurretConfig;
};

USTRUCT(BlueprintType)
struct FNSProjectileAbilityConfig
{
	GENERATED_BODY()
	
	// Projectile 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectileAbilityConfig")
	EProjectileType ProjectileType = EProjectileType::Explosive;
	
	// 폭발물타입 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectileAbilityConfig|Explosive",
		meta = (EditCondition = "ProjectileType == EProjectileType::Explosive", EditConditionHides))
	FNSExplosiveTypeConfig ExplosiveTypeConfig;
	
	// 터렛소환타입 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectileAbilityConfig|TurretSpawner",
		meta = (EditCondition = "ProjectileType == EProjectileType::TurretSpawner", EditConditionHides))
	FNSTurretSpawnerTypeConfig TurretSpawnerTypeConfig;

	// CombatStat 값을 GameplayEffect SetByCaller 값으로 넘기기 위한 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectileAbilityConfig|SetByCaller")
	TArray<FNSSetByCallerFromCombatStat> SetByCallerMappings;

	// CombatStat 값을 투척물 런타임 로직 값으로 넘기기 위한 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectileAbilityConfig|RuntimeStats")
	TArray<FNSRuntimeStatFromCombatStat> RuntimeStatMappings;
};


/**
 * Projectile을 포물선 형태로 던지기 위한 GA
 * Input이 Hold되는 동안 궤적 + 탄착지점 프리뷰를 보여주고 
 * Input이 Release 되는 순간 프리뷰를 제거하고 Projectile을 스폰
 */
UCLASS(Abstract, Blueprintable)
class NEOSANCTUM_API UGA_ThrowProjectile : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_ThrowProjectile();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UFUNCTION()
	void OnThrowMontageCompleted();

	UFUNCTION()
	void OnThrowMontageInterrupted();

	UFUNCTION()
	void OnAttachProjectileEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnThrowProjectileEventReceived(FGameplayEventData Payload);

protected:
	void StartGameplayEventTasks();
	void AttachHeldMesh();
	void DestroyHeldMesh();
	
	void SpawnProjectileAtAimPoint(const FVector& AimPoint);
	
protected:
	FTransform GetProjectileSpawnTransform() const;
	
	bool TryBuildProjectileAimTrace(FHitResult& OutHitResult) const;
	FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResult(const FHitResult& HitResult) const;
	void OnTargetDataReadyCallback(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle,
		FGameplayTag ApplicationTag
	);
	void OnThrowProjectileTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	bool TryGetAimPointFromTargetData(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle,
		FVector& OutAimPoint
	) const;

protected:
	// CombatStat 조회 기준 Ability 태그 결정
	bool TryGetCombatStatAbilityTag(FGameplayTag& OutAbilityTag) const;
	// CombatStat 기반 payload 갱신
	void RebuildCombatStatPayloads();
	// GE SetByCaller payload 생성
	void RebuildSetByCallerMagnitudes(const FGameplayTag& AbilityTag);
	// 투척물 런타임 payload 생성
	void RebuildRuntimeStatMagnitudes(const FGameplayTag& AbilityTag);
	
	void AddDeactivateHandIKTag();
	void RemoveDeactivateHandIKTag();
	
protected:
	// Ability 설정모음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Config")
	FNSProjectileAbilityConfig ProjectileAbilityConfig;

	// CombatStat 조회에 사용할 Ability 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Config")
	FGameplayTag CombatStatAbilityTag;

	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	TObjectPtr<UAnimMontage> AnimMontage;
	
	// Release 단계의 애니메이션 몽타주 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	FName ReleaseSectionName = TEXT("Release");
	
	// Hold 단계에서 손에 메쉬를 붙히는 타이밍을 정해주는 Event Tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	FGameplayTag AttachTag;
	
	// Release 단계에서 손에 붙어있던 메쉬를 제거하고 Projectile을 던지는 타이밍을 정해주는 Event Tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	FGameplayTag ReleaseTag;
	
protected:
	// Hold 단계에서 손에 붙힐 메쉬
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	TObjectPtr<UStaticMesh> HoldStaticMesh;
	
	// Hold 단계에서 메쉬를 붙힐 소켓
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	FName HoldAttachSocketName = TEXT("Weapon_l");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	FTransform HoldRelativeTransform = FTransform::Identity;

protected:
	// 발사할 Projectile 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	TSubclassOf<ANSThrowProjectileBase> ProjectileClass;

	// Projectile 발사 위치 소켓
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	FName ProjectileSpawnSocketName = TEXT("Weapon_l");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Aim")
	float AimTraceRange = 10000.0f;

private:
	// 몽타주 재생 AbilityTask
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ThrowMontageTask;
	
	// 메쉬를 손에 붙히는 타이밍 GameplayEvent를 기다리는 Task	
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttachProjectileEventTask;
	
	// Projectile 액터를 발사하면서 손에 붙어있던 메쉬를 제거하는 타이밍 GameplayEvent를 기다리는 Task
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ThrowProjectileEventTask;
	
	// 캐릭터에 잠깐 붙히게 될 MeshComponent
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HoldMeshComponent;

	// GE에 전달할 SetByCaller payload
	UPROPERTY(Transient)
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

	// 투척물 로직에 전달할 runtime payload
	UPROPERTY(Transient)
	TArray<FNSCombatStatMagnitude> RuntimeStatMagnitudes;

	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;

	bool bDeactivateHandIKTagAdded = false;
};

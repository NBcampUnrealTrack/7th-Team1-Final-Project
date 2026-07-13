// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "GA_MotherShipCloakDeployRun.generated.h"

class ANSEnemyCharacterBase;
class ANSBossMotherShip;
class ANSBossArenaBounds;
class UNSEnemyData;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;


USTRUCT(BlueprintType)
struct FNSAirDropSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "AirDrop")
	TSubclassOf<ANSEnemyCharacterBase> CharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "AirDrop")
	TObjectPtr<UNSEnemyData> EnemyData;

	UPROPERTY(EditDefaultsOnly, Category = "AirDrop", meta = (ClampMin = "0"))
	int32 CountPerWave = 1;
};

UCLASS()
class NEOSANCTUM_API UGA_MotherShipCloakDeployRun : public UGA_EnemyAttackBase
{
	GENERATED_BODY()
	
	public:
	UGA_MotherShipCloakDeployRun();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 몽타주 완료 자동 Finish 방지 (BombingRun과 동일 안전장치)
	virtual void HandleAttackMontageCompleted() override;

private:
	enum class ECloakDeployLeg : uint8
	{
		CloakAscend,
		TraverseAndDeploy,
		ReturnUncloak,
	};

	void BeginLeg(ECloakDeployLeg NewLeg);
	void PollLegProgress();

	// TraverseDuration/SpawnInterval 만큼의 DeployWave 타이머를 예약
	void ScheduleDeployWaves();

	// 보스 현재 위치 기준으로 SpawnEntries를 순회하며 실제 스폰 수행
	void DeployWave(int32 WaveIndex);

	// BaseLocation 주변 SpawnScatterRadius 내에서, 이번 웨이브에서 이미 쓴 위치와 안 겹치는 오프셋 위치를 계산
	FVector ComputeScatteredSpawnLocation(
		const FVector& BaseLocation,
		const TArray<FVector>& PlacedLocationsThisWave) const;

	// 이전 레그 몽타주 태스크를 정리하고 새 몽타주 재생 (완료 델리게이트로 레그 전환/Finish 하지 않음)
	void PlayLegMontage(UAnimMontage* Montage);

	UFUNCTION()
	void OnLegMontageEnded();

	void EnterCloak();
	void ExitCloak();

	ANSBossMotherShip* GetBossMotherShip() const;
	ANSBossArenaBounds* GetArenaBounds() const;

	// ArenaBounds/Boss/FlyingLocomotion/SpawnEntries 유효성 검증
	bool ValidateAttackContext() const;

	int32 GetWaveCount() const;

private:
	// ---- Move ----
	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Move", meta = (ClampMin = "0.0"))
	float RunAltitude = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Move", meta = (ClampMin = "1.0"))
	float AscendSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Move", meta = (ClampMin = "1.0"))
	float RunSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Move", meta = (ClampMin = "1.0"))
	float DescendSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Move", meta = (ClampMin = "0.01"))
	float LegPollInterval = 0.1f;

	// ---- Deploy ----
	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Deploy", meta = (ClampMin = "0.1"))
	float TraverseDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Deploy", meta = (ClampMin = "0.05"))
	float SpawnInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Deploy", meta = (ClampMin = "0.0"))
	float SpawnScatterRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Deploy")
	TArray<FNSAirDropSpawnEntry> SpawnEntries;

	// ---- Montage ----
	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Montage")
	TObjectPtr<UAnimMontage> AscendDescendMontage;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Montage")
	TObjectPtr<UAnimMontage> TraverseMontage;

	// ---- Cloak ----
	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Cloak")
	FGameplayTag CloakCueTag;

	// ---- Debug ----
	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "CloakDeployRun|Debug", meta = (ClampMin = "0.0"))
	float DrawTime = 2.f;

	// ---- Runtime ----
	ECloakDeployLeg CurrentLeg = ECloakDeployLeg::CloakAscend;
	TWeakObjectPtr<ANSBossArenaBounds> CachedArenaBounds;
	bool bCloaked = false;
	bool bAllWavesDeployed = false;

	// ActivateAbility 시점에 캐시한 원래 유지 고도. DescendUncloak 레그가 이 고도로 복귀
	float CapturedAltitude = 600.f;

	
	FTimerHandle LegPollTimerHandle;
	TArray<FTimerHandle> DeployWaveTimerHandles;

	TWeakObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentLegMontageTask;
};

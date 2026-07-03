// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/TimerHandle.h"
#include "NSBossPawnBase.h"
#include "NSBossMotherShip.generated.h"

class ANSEnemyDrone;
class UNSDronePoolManager;
class UNSFlyingLocomotionComponent;
class UNSEnemyData;
class UGameplayAbility;

UCLASS()
class NEOSANCTUM_API ANSBossMotherShip : public ANSBossPawnBase
{
	GENERATED_BODY()

public:
	ANSBossMotherShip();

protected:
	virtual void BeginPlay() override;

#pragma region SpawnDrone
public:
	// 드론 소환 유지 루프 시작 (전투 진입 시)
	void StartDroneSpawnLoop();

	// 드론 소환 유지 루프 정지 (사망 등)
	void StopDroneSpawnLoop();

	// GA_BossSpawnDrone 의 Notify가 호출하는 top-up 진입점 (부족분만 채움)
	void SpawnEnemyDrone();

private:
	// 타이머 콜백: 연출 중/최대치면 skip, 아니면 소환 어빌리티 발동
	void TickDroneSpawn();

	// 현재 페이즈 기준 최대 드론 수 반환
	int32 GetCurrentMaxDrones() const;

	// 죽었거나 무효한 드론을 풀로 반환하고 로스터에서 제거
	void PruneActiveDrones();

	// Count개의 소환 위치(월드 Transform)를 계산
	bool BuildDroneSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const;

	// 풀에서 드론 1기를 꺼내 배치 (없으면 풀이 신규 생성)
	ANSEnemyDrone* AcquireDroneFromPool(const FTransform& SpawnTransform);

private:
	// ---- Config ----
	// 스폰할 드론 Pawn 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TSubclassOf<ANSEnemyDrone> DroneClass;

	// 스폰된 드론에 주입할 EnemyData (스탯/외형/공격 테이블)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TObjectPtr<UNSEnemyData> DroneEnemyData;

	// 타이머가 주기적으로 발동할 소환 어빌리티 (GA_BossSpawnDrone)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TSubclassOf<UGameplayAbility> SpawnDroneAbilityClass;

	// 페이즈 태그별 최대 드론 수 (예: Phase.1 → 8, Phase.2 → 12)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TMap<FGameplayTag, int32> MaxDronesByPhase;

	// MaxDronesByPhase에 현재 페이즈가 없을 때 쓸 기본 최대치
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 DefaultMaxDrones = 8;

	// 소환 유지 점검 주기(초)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0.1"))
	float SpawnInterval = 5.f;

	// 소환 위치로 쓸 메시 소켓 이름들 (비어 있으면 함선 주위 원형 배치로 fallback)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TArray<FName> SpawnSocketNames;

	// 소켓이 없을 때 함선 주위로 드론을 배치할 반경
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0"))
	float SpawnRingRadius = 400.f;

	// ---- Runtime ----
	// 보스가 소유하는 드론 전용 풀
	UPROPERTY(Transient)
	TObjectPtr<UNSDronePoolManager> DronePool;

	// 현재 살아있는(전개된) 드론 로스터. 파괴 시 자동 무효화되도록 WeakPtr 사용
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ANSEnemyDrone>> ActiveDrones;

	// 소환 유지 타이머 핸들
	FTimerHandle DroneSpawnTimerHandle;
#pragma endregion

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
	TObjectPtr<UNSFlyingLocomotionComponent> FlyingLocomotionComponent;
};
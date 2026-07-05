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
class ANSBossControlDevice;

UCLASS()
class NEOSANCTUM_API ANSBossMotherShip : public ANSBossPawnBase
{
	GENERATED_BODY()

public:
	ANSBossMotherShip();
	
	void NotifySummonEnded() { bSummonInFlight = false; }

protected:
	virtual void BeginPlay() override;
	virtual UNSFlyingLocomotionComponent* GetFlyingLocomotion() const override;
	
#pragma region SpawnDrone
public:
	// 전투 진입 시 패턴 개시 (초기 웨이브 + 유지 타이머). 기존 StartDroneSpawnLoop 대체
	void StartDronePattern();

	// 패턴 종료 (사망/전투 종료). 기존 StopDroneSpawnLoop 대체
	void StopDronePattern();

	// [GA_BossSpawnDrone Notify가 호출] 성숙한 재생성 예약을 최대치 한도 내에서 실제 스폰
	void SpawnMaturedDrones();

private:
	// 유지 점검 틱: 페이즈 전환 감지 + 성숙 예약 소환 요청. 기존 TickDroneSpawn 대체
	void MaintenanceTick();

	// 현재 최대치까지 '즉시 성숙(=Now)' 예약을 채워 넣음 (초기 웨이브/페이즈 재충원)
	void QueueFullWave();

	// 재생성 예약 1건 추가 (드론 사망 시): Now + RespawnDelay
	void ScheduleRespawn();

	// 성숙 예약이 있고 자리가 있으면 소환 어빌리티 발동 (중복 발동 방지 포함)
	void RequestSummonAbility();

	// 페이즈 전환: 전개된 드론 전원 회수 + 재생성 예약 전부 폐기
	void RecallAllDrones();

	// 드론 사망 콜백 (StateComponent->OnDeathStarted 바인딩, 서버 전용)
	void HandleDroneDied(TWeakObjectPtr<ANSEnemyDrone> DeadDrone);

	// 현재 살아있는(전개된) 드론 수 (WeakPtr 유효분만)
	int32 GetActiveDroneCount() const;

	// NowSeconds 기준 성숙(만료)된 예약 개수
	int32 CountMaturedRespawns(double NowSeconds) const;

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
	float SpawnInterval = 1.f;

	// 소환 위치로 쓸 메시 소켓 이름들 (비어 있으면 함선 주위 원형 배치로 fallback)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TArray<FName> SpawnSocketNames;

	// 소켓이 없을 때 함선 주위로 드론을 배치할 반경
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0"))
	float SpawnRingRadius = 400.f;

	// 드론 사망 연출(디졸브)을 보여준 뒤 풀 반환까지 지연(초). 0이면 즉시 반환
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float ReturnToPoolDelay = 2.f;
	
	// 파괴된 드론 1기의 개별 재생성 대기 시간(초). 스펙: 30
	UPROPERTY(EditDefaultsOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 30.f;
	
	// ---- Runtime ----
	// 개별 재생성 예약: 각 원소 = "이 월드 시각이 지나면 1기 재생성 가능"
	UPROPERTY(Transient)
	TArray<double> PendingRespawnTimes;

	// 직전 틱에서 관측한 페이즈 태그 (전환 감지용)
	FGameplayTag LastObservedPhaseTag;

	// 페이즈 전환 종료 후 새 최대치로 재충원해야 하는지
	bool bPendingPhaseRefill = false;

	// 소환 어빌리티 발동 후 Notify(실제 스폰) 대기 중인지 (중복 발동 방지)
	bool bSummonInFlight = false;
	
	// 보스가 소유하는 드론 전용 풀
	UPROPERTY(Transient)
	TObjectPtr<UNSDronePoolManager> DronePool;

	// 현재 살아있는(전개된) 드론 로스터. 파괴 시 자동 무효화되도록 WeakPtr 사용
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ANSEnemyDrone>> ActiveDrones;
	
	void ReturnDroneAfterDelay(TWeakObjectPtr<ANSEnemyDrone> Drone);

	// 소환 유지 타이머 핸들
	FTimerHandle DroneSpawnTimerHandle;
#pragma endregion

#pragma region ControlDevice
public:
	// 모든 제어장치 파괴 완료(보스 피격 가능) 여부. AI/연출 조회용
	bool AreControlDevicesCleared() const { return bControlDevicesCleared; }

private:
	// EquipParts로 스폰된 제어장치를 PartId로 조회·등록하고 초기 무적을 적용
	void InitControlDevices();

	// 제어장치 1기 파괴 콜백. 생존 수 감소, 0이면 보스 무적 해제
	void HandleControlDeviceDestroyed(ANSBossControlDevice* DestroyedDevice);

	// 보스 ASC에 State.Invincible LooseTag 부여(데미지 차단)
	void ApplyBossInvincibility();

	// 보스 ASC에서 State.Invincible LooseTag 제거(데미지 허용)
	void ClearBossInvincibility();

private:
	// ---- Config ----
	// DT_EnemyParts에서 제어장치로 쓸 PartId 목록 (예: ControlDevice_01~04)
	UPROPERTY(EditDefaultsOnly, Category = "ControlDevice")
	TArray<FName> ControlDevicePartIds;

	// ---- Runtime ----
	// 등록된 제어장치들 (파괴 후 자동 무효화되도록 WeakPtr)
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ANSBossControlDevice>> ControlDevices;

	// 현재 살아있는 제어장치 수
	int32 AliveControlDeviceCount = 0;

	// 모든 제어장치가 파괴돼 보스가 피격 가능해졌는지
	bool bControlDevicesCleared = false;
#pragma endregion
	
#pragma region PhaseTransition
public:
	// [TransitionGA 연출 시작 시] 무적 진입 + (추후)이동 정지
	void BeginPhase2Transition();

	// [TransitionGA 연출 종료 시] 쉴드 드레인 / 보스 쉴드 / 무적 해제 / 이동 허용
	void CompletePhase2Transition();

	// 보스 직접 이동 허용 여부 (AIController가 이동 게이트로 사용 예정)
	bool IsBossMovementUnlocked() const { return bBossMovementUnlocked; }
	
private:
	// 필드의 모든 플레이어 쉴드를 RemoveRatio(0~1)만큼 제거
	void DrainAllPlayerShields(float RemoveRatio);

	// 보스에게 Phase2 방어막 부여
	void GrantBossShield();

private:
	// ---- Config ----
	// Phase2 진입 시 보스가 얻는 방어막 총량
	UPROPERTY(EditDefaultsOnly, Category = "PhaseTransition", meta = (ClampMin = "0.0"))
	float Phase2ShieldAmount = 5000.f;

	// 연출 종료 시 각 플레이어에게서 제거할 쉴드 비율 (0.7 = 70%)
	UPROPERTY(EditDefaultsOnly, Category = "PhaseTransition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayerShieldDrainRatio = 0.7f;

	// ---- Runtime ----
	// 보스 이동 허용 플래그(연출 종료 후 true). 실제 이동 로직은 AIController에서 게이트
	bool bBossMovementUnlocked = false;
#pragma endregion
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
	TObjectPtr<UNSFlyingLocomotionComponent> FlyingLocomotionComponent;
};
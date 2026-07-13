// GA_MotherShipBombingRun.h
#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "GA_MotherShipBombingRun.generated.h"

class ANSBossMotherShip;
class ANSBossArenaBounds;
struct FNSEnemyAttackRow;

UCLASS()
class NEOSANCTUM_API UGA_MotherShipBombingRun : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_MotherShipBombingRun();

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
	// 몽타주 완료 자동 Finish 방지 (FlyingBurst와 동일)
	virtual void HandleAttackMontageCompleted() override;

private:
	enum class EBombingRunLeg : uint8
	{
		Ascend,
		Traverse,
		HoldAtEntrance,
		Return,
	};

	// 레그 전환: 상태 갱신 + 필요 시 BeginScriptedMove + 폴링 타이머 시작
	void BeginLeg(EBombingRunLeg NewLeg);

	// 저빈도 타이머 콜백: HasReachedScriptedDest() 체크 후 다음 레그 전환
	void PollLegProgress();

	// Traverse 완주 후 호출: N개 존 경고 데칼을 순차 예약, 완료 시 StartDetonationPhase 예약
	void StartWarningPhase();

	// 경고 페이즈 종료 후 호출: N개 존 폭발을 순차 예약
	void StartDetonationPhase();

	// t = i*ShotInterval: 존 i 경고 데칼 Cue 부여
	void ShowZoneWarning(int32 ZoneIndex);

	// t = i*ShotInterval + ImpactDelay: 미사일/착탄 Cue + 박스 오버랩 데미지 + 경고 Cue 제거
	void DetonateZone(int32 ZoneIndex);

	// 활성 경고 Cue 전부 제거
	void ClearAllZoneWarningCues();

	// 존 타이머(경고+폭발) 전부 Clear
	void ClearAllZoneTimers();

	float CalculateBombardDamage(const FNSEnemyAttackRow& AttackRow) const;
	bool ApplyBombardDamageToTarget(AActor* TargetActor, const FHitResult& HitResult, float Damage);

	ANSBossMotherShip* GetBossMotherShip() const;
	ANSBossArenaBounds* GetArenaBounds() const;
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// ArenaBounds/AttackRow/로코모션 무효 시 false (호출부에서 즉시 Cancel)
	bool ValidateAttackContext() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Move", meta = (ClampMin = "0.0"))
	float RunAltitude = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Move", meta = (ClampMin = "1.0"))
	float AscendSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Move", meta = (ClampMin = "1.0"))
	float RunSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Move", meta = (ClampMin = "1.0"))
	float ReturnSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Move", meta = (ClampMin = "0.01"))
	float LegPollInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Zone", meta = (ClampMin = "1.0"))
	float ZoneHeight = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Zone")
	TArray<FLinearColor> ZoneColors = {
		FLinearColor::Red, FLinearColor::Green, FLinearColor::Blue, FLinearColor(1.f, 0.5f, 0.f)
	};

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Cue")
	TArray<FGameplayTag> WarningCueTags;  

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Cue")
	FGameplayTag MissileCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "BombingRun|Cue")
	FGameplayTag ImpactCueTag;

	EBombingRunLeg CurrentLeg = EBombingRunLeg::Ascend;
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;
	TWeakObjectPtr<ANSBossArenaBounds> CachedArenaBounds;

	// ActivateAbility 시점에 캐시한 원래 유지 고도. Return 레그가 이 고도로 복귀
	float CapturedAltitude = 600.0f;
	
	FTimerHandle LegPollTimerHandle;
	FTimerHandle DetonationPhaseTimerHandle;
	TArray<FTimerHandle> ZoneWarningTimerHandles;
	TArray<FTimerHandle> ZoneImpactTimerHandles;

	// 아직 경고 Cue가 남아있는 존 인덱스 (취소 시 일괄 Remove용)
	TArray<int32> ActiveWarningZoneIndices;
	
	void DrawDebugZone(int32 ZoneIndex) const;
};
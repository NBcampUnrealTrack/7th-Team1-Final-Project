// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_EnemyAttackBombard.generated.h"

class ANSBossAIController;
class UNSEnemyPartComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.06
 * 
 * 클래스 개요 : TitanWalker의 포격 공격을 처리하는 Gameplay Ability
 * AttackMontage의 GameplayEvent 시점에 BombardData 기준 착탄 지점을 생성하고,
 * ImpactDelay 이후 ImpactRadius 범위 데미지를 적용
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackBombard : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackBombard();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual void InitializeAttack() override;
	virtual void HandleAttackEvent(const FGameplayEventData& Payload) override;
	virtual void HandleAttackMontageCompleted() override;

private:
	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// AnimNotify GameplayEvent 이후 포격 패턴을 시작하는 함수
	void StartBombardVolley();

	// ShotIndex 기준 포격 한 발의 착탄 경고와 Impact 예약을 처리하는 함수
	void FireBombardShot(int32 ShotIndex);

	// ImpactDelay 이후 실제 범위 데미지를 적용하는 함수
	void ApplyBombardImpact(FVector ImpactLocation);

	// ShotIndex 기준 공격 대상 Actor를 선택하는 함수
	AActor* ResolveBombardTarget(int32 ShotIndex) const;

	// TargetActor 주변에서 실제 착탄 위치를 계산하는 함수
	FVector ResolveImpactLocation(AActor* TargetActor) const;

	// AttackId와 연결된 포격 Muzzle 위치를 반환하는 함수
	FVector ResolveMuzzleLocation(int32 ShotIndex) const;

	// TargetActor의 Bounds 기준 판정 위치를 반환하는 함수
	FVector GetTargetCheckLocation(AActor* TargetActor) const;

	// Impact 위치 주변의 데미지 대상 목록을 수집하는 함수
	void CollectTargetsAtImpact(
		const FVector& ImpactLocation,
		TSet<TObjectKey<AActor>>& OutTargets) const;

	// TargetActor가 데미지를 받을 수 있는 유효한 적대 대상인지 확인하는 함수
	bool IsValidDamageTarget(AActor* TargetActor) const;

	// Source BaseDamage와 AttackRow DamageScale 기준 데미지를 계산하는 함수
	float CalculateBombardDamage(const FNSEnemyAttackRow& AttackRow) const;

	// TargetActor에게 Bombard DamageEffect를 적용하는 함수
	bool ApplyBombardDamageToTarget(
		AActor* TargetActor,
		const FVector& ImpactLocation,
		const FNSEnemyAttackRow& AttackRow);

	// ImpactRadius 값을 반환하는 함수
	float GetImpactRadius(const FNSEnemyAttackRow& AttackRow) const;

	// 착탄 전 경고 Debug를 표시하는 함수
	void DrawDebugBombardWarning(
		const FVector& MuzzleLocation,
		const FVector& ImpactLocation,
		const FNSEnemyAttackRow& AttackRow) const;

	// 실제 착탄 Debug를 표시하는 함수
	void DrawDebugBombardImpact(
		const FVector& ImpactLocation,
		const FNSEnemyAttackRow& AttackRow) const;

	// 예약된 Shot/Impact Timer를 정리하는 함수
	void ClearBombardTimers();

	// 몽타주와 예약된 착탄이 모두 끝났을 때 Ability를 종료하는 함수
	void TryFinishBombardAbility();

private:
	// 착탄 위치 보정에 사용할 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Bombard")
	TEnumAsByte<ECollisionChannel> ImpactTraceChannel = NSCollisionChannels::ExplosionTrace;

	// 착탄 지점을 지면에 맞추기 위해 위쪽으로 올릴 Trace 높이
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Bombard", meta = (ClampMin = "0.0"))
	float GroundTraceHeight = 3000.0f;

	// 착탄 지점을 지면에 맞추기 위해 아래쪽으로 내릴 Trace 깊이
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Bombard", meta = (ClampMin = "0.0"))
	float GroundTraceDepth = 6000.0f;

	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// 포격 패턴이 이미 시작됐는지 여부
	bool bVolleyStarted = false;

	// AttackMontage 재생이 끝났는지 여부
	bool bMontageCompleted = false;

	// 현재까지 발사 처리를 마친 포격 수
	int32 FiredShotCount = 0;

	// 아직 데미지 적용이 끝나지 않은 Impact 수
	int32 PendingImpactCount = 0;

	// Shot 예약 Timer 목록
	TArray<FTimerHandle> ShotTimerHandles;

	// Impact 예약 Timer 목록
	TArray<FTimerHandle> ImpactTimerHandles;
};

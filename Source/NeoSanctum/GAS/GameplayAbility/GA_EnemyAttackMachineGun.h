// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_EnemyAttackMachineGun.generated.h"

class ANSBossAIController;
class UNSEnemyPartComponent;
class UNSProjectileManagerComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.06
 *
 * 클래스 개요 : Enemy Part Row의 Muzzle을 사용해 ProjectileManager 기반 기관총 투사체를 연속 발사하는 공격 Ability
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackMachineGun : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackMachineGun();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InitializeAttack() override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 현재 Control Rig 조준이 반영된 Muzzle Transform 목록을 반환하는 함수
	void GetCurrentMuzzleTransforms(TArray<FTransform>& OutTransforms) const;

private:
	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// GameState의 ProjectileManagerComponent를 반환하는 함수
	UNSProjectileManagerComponent* GetProjectileManager() const;

	// 현재 공격 대상 Actor를 반환하는 함수
	AActor* ResolveAttackActor() const;

	// AimMode와 타깃 상태 기준으로 조준 위치를 계산하는 함수
	FVector ResolveAimPoint(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// 조준 위치와 SpreadAngle 기준으로 최종 발사 방향을 계산하는 함수
	FVector ResolveFireDirection(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// 발사 방향에 랜덤 퍼짐을 적용하는 함수
	FVector ApplySpread(
		const FVector& Direction,
		float SpreadAngle) const;

	// Source BaseDamage와 AttackRow DamageScale 기준 최종 데미지를 계산하는 함수
	float CalculateProjectileDamage(const FNSEnemyAttackRow& AttackRow) const;

	// 첫 발사 이벤트 수신 후 연속 발사를 시작하는 함수
	void StartBurst();

	// ProjectileManager에 투사체 1발 발사를 요청하는 함수
	void FireNextProjectile();

	// 연속 발사 타이머를 정리하는 함수
	void ClearBurstTimer();

	// DebugData 기준으로 발사 방향을 표시하는 함수
	void DrawDebugFire(
		const FNSEnemyAttackRow& AttackRow,
		const FVector& Start,
		const FVector& Direction) const;
	
	// WarnTime과 조준 수렴을 기다린 뒤 연속 발사를 시작하는 함수
	void StartPreAimOrBurst();

	// PreAim 대기 중 조준 수렴 여부를 반복 확인하는 함수
	void TryStartBurstAfterPreAim();

	// AnimInstance 기준으로 현재 조준이 발사 가능한 수준인지 확인하는 함수
	bool IsAimReadyForFire() const;

private:
	// ProjectileManager Sweep에 사용할 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	TEnumAsByte<ECollisionChannel> ProjectileTraceChannel = NSCollisionChannels::EnemyProjectile;

	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// 현재 Ability에서 발사한 투사체 수
	int32 FiredCount = 0;

	// 다음에 사용할 Muzzle Transform 인덱스
	int32 NextMuzzleIndex = 0;

	// 한 Ability 활성화 중 연속 발사가 이미 시작됐는지 여부
	bool bBurstStarted = false;

	// FireInterval 반복 발사를 위한 타이머 핸들
	FTimerHandle BurstTimerHandle;
	
	// 조준 완료로 인정할 최대 각도 오차
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Aim", meta = (ClampMin = "0.0"))
	float AimReadyToleranceDegrees = 6.0f;

	// WarnTime 이후에도 조준이 맞지 않을 때 최대 추가 대기할 시간
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Aim", meta = (ClampMin = "0.0"))
	float MaxPreAimWaitTime = 1.0f;

	// PreAim 중 조준 완료 여부를 확인하는 주기
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Aim", meta = (ClampMin = "0.01"))
	float PreAimPollInterval = 0.03f;

	// PreAim 시작 시각
	float PreAimStartTime = 0.0f;

	// PreAim 대기 Timer
	FTimerHandle PreAimTimerHandle;
};

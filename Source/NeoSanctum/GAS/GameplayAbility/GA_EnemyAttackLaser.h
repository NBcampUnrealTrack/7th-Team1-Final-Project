// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_EnemyAttackLaser.generated.h"

class UNSEnemyPartComponent;
class ANSBossAIController;
class UAudioComponent;
class UNiagaraComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.06
 * 
 * 클래스 개요 : TitanWalker의 Center Laser 포신 TraceSocket을 기준으로 직선형 지속 레이저 판정을 처리하는 공격 Ability
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackLaser : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackLaser();

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

private:
	struct FNSLaserBeam
	{
		// 레이저 판정 시작 위치
		FVector Start = FVector::ZeroVector;

		// 레이저 판정 끝 위치
		FVector End = FVector::ZeroVector;
	};

	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 현재 공격 Row 기준 레이저 Beam 목록을 반환하는 함수
	void GetCurrentLaserBeams(TArray<FNSLaserBeam>& OutBeams) const;

	// WarnTime 이후 실제 레이저 데미지 루프를 시작하는 함수
	void StartLaser();

	// 레이저 데미지 Tick 타이머를 시작하는 함수
	void BeginLaserDamage();

	// 레이저 데미지 Tick을 처리하는 함수
	void TickLaserDamage();

	// 레이저 지속 시간이 끝났을 때 Ability를 정상 종료하는 함수
	void CompleteLaser();

	// 레이저 관련 Timer를 정리하는 함수
	void ClearLaserTimers();

	// Beam 하나의 Sweep 범위 안에 들어온 타깃을 수집하는 함수
	void CollectTargetsForBeam(
		const FNSLaserBeam& Beam,
		TSet<TObjectKey<AActor>>& OutTargets) const;

	// TargetActor가 데미지를 받을 수 있는 유효한 대상인지 확인하는 함수
	bool IsValidDamageTarget(AActor* TargetActor) const;

	// TargetActor의 Bounds 기준 판정 위치를 반환하는 함수
	FVector GetTargetCheckLocation(AActor* TargetActor) const;

	// Source BaseDamage와 AttackRow DamageScale 기준 Tick 데미지를 계산하는 함수
	float CalculateLaserDamage(const FNSEnemyAttackRow& AttackRow) const;

	// TargetActor에게 Laser DamageEffect를 적용하는 함수
	bool ApplyLaserDamageToTarget(AActor* TargetActor, const FNSEnemyAttackRow& AttackRow);

	// AttackRow 기준 레이저 사거리를 반환하는 함수
	float GetLaserRange(const FNSEnemyAttackRow& AttackRow) const;

	// AttackRow 기준 레이저 판정 반경을 반환하는 함수
	float GetLaserRadius(const FNSEnemyAttackRow& AttackRow) const;

	// DebugData 기준으로 레이저 직선 범위를 표시하는 함수
	void DrawDebugLaserBeam(
		const FNSLaserBeam& Beam,
		const FNSEnemyAttackRow& AttackRow) const;

private:
	// Laser Sweep에 사용할 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Laser")
	TEnumAsByte<ECollisionChannel> LaserTraceChannel = NSCollisionChannels::EnemyWeaponTrace;

	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// WarnTime 대기 Timer
	FTimerHandle LaserStartTimerHandle;

	// Laser Tick 반복 Timer
	FTimerHandle LaserTickTimerHandle;

	// Laser 종료 Timer
	FTimerHandle LaserEndTimerHandle;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic")
	FName LaserChargeSoundID = FName(TEXT("Monster_TitanWalker_Laser_Charge"));

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic")
	FName LaserFireSoundID = FName(TEXT("Monster_TitanWalker_Laser_Beam"));

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic")
	FName LaserChargeVFXID = FName(TEXT("Monster_TitanWalker_Laser_Charge"));

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic")
	FName LaserBeamVFXID = FName(TEXT("Monster_TitanWalker_Laser_Beam"));

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic")
	FName LaserChargeDurationParameterName = FName(TEXT("User.ChargeDuration"));
	
	// Beam Niagara가 사용할 월드 좌표 끝점 User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX")
	FName LaserBeamEndParameterName = FName(TEXT("User.Beam End"));

	// Beam Niagara에 적용할 고정 스케일 값을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.01"))
	float LaserBeamVFXScale = 1.0f;

	// Beam Niagara의 표시 두께 User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX")
	FName LaserBeamWidthParameterName = FName(TEXT("User.Beam Width"));
	
	// AreaData.Radius를 Niagara Beam Width로 변환할 때 곱하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.0"))
	float LaserBeamWidthRadiusMultiplier = 2.0f;

	// Niagara Beam Width의 최소 표시 폭을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.0"))
	float LaserBeamMinVisualWidth = 4.0f;
	

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveLaserChargeAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveLaserFireAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveLaserChargeVFXComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveLaserBeamVFXComponents;

	void StartLaserChargeCosmetics();
	void StopLaserChargeCosmetics();
	void StartLaserFireCosmetics(const TArray<FNSLaserBeam>& Beams);
	void UpdateLaserBeamCosmetics(const TArray<FNSLaserBeam>& Beams);
	void StopLaserCosmetics();
	
	// AttackRow의 AreaData.Radius 기준으로 Niagara Beam Width 값을 계산하는 함수
	float GetLaserBeamVisualWidth(const FNSEnemyAttackRow& AttackRow) const;
	
protected:
	// Charge 사운드 안에 Beam 사운드까지 포함되어 있으면 Beam 시작 시 별도 사운드를 재생하지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|Sound")
	bool bLaserFireSoundIncludedInChargeSound = true;

	// WarnTime 동안 Charge VFX를 소켓 위치에 따라 갱신하는 간격
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.01"))
	float LaserChargeVFXUpdateInterval = 0.03f;

	// WarnTime 동안 Charge VFX 위치 갱신에 사용하는 타이머
	FTimerHandle LaserChargeVFXUpdateTimerHandle;

	// Charge VFX를 현재 레이저 소켓 위치와 방향으로 갱신하는 함수
	void UpdateLaserChargeCosmetics();

	// Charge VFX만 정리하는 함수
	void StopLaserChargeVFX();

	// Charge 사운드만 정리하는 함수
	void StopLaserChargeSound();
	
private:
	// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// 현재 공격 대상 Actor를 반환하는 함수
	AActor* ResolveAttackActor() const;

	// AimMode와 타깃 상태 기준으로 레이저 조준 위치를 계산하는 함수
	FVector ResolveLaserAimPoint(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// Muzzle 위치와 공격 대상 기준으로 레이저 진행 방향을 계산하는 함수
	FVector ResolveLaserDirection(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;
};

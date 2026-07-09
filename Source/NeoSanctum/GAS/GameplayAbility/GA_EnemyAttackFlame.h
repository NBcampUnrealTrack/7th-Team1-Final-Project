// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_EnemyAttackFlame.generated.h"

class ANSBossAIController;
class UNSEnemyPartComponent;
class UNiagaraComponent;
class UAudioComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.06
 *
 * 클래스 개요 : Enemy Part Row의 화염 방사 포인트를 기준으로 Cone 범위 데미지를 반복 적용하는 지속형 공격 Ability
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackFlame : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackFlame();

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
	struct FNSFlameEmitter
	{
		// 화염 판정 시작 위치
		FVector Start = FVector::ZeroVector;

		// 화염 판정 진행 방향
		FVector Direction = FVector::ForwardVector;
	};

	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// 현재 공격 Row 기준 화염 방사 포인트 목록을 반환하는 함수
	void GetCurrentFlameEmitters(TArray<FNSFlameEmitter>& OutEmitters) const;

	// 화염 지속 판정을 시작하는 함수
	void StartFlame();

	// 화염 데미지 Tick을 처리하는 함수
	void TickFlameDamage();

	// 화염 지속 시간이 끝났을 때 Ability를 정상 종료하는 함수
	void CompleteFlame();

	// 화염 관련 Timer를 정리하는 함수
	void ClearFlameTimers();

	// 현재 유효 사거리 안에 들어온 대상만 수집하는 함수
	void CollectTargetsForEmitter(
		const FNSFlameEmitter& Emitter, 
		float EffectiveRange, 
		TSet<AActor*>& OutTargets) const;


	// 대상 위치가 현재 유효 사거리의 화염 원뿔 안에 있는지 확인합니다.
	bool IsLocationInsideCone(
		const FVector& Origin, 
		const FVector& Direction, 
		const FVector& TargetLocation, 
		const FNSEnemyAttackRow& AttackRow, 
		float EffectiveRange) const;

	// TargetActor가 데미지를 받을 수 있는 유효한 적대 대상인지 확인하는 함수
	bool IsValidDamageTarget(AActor* TargetActor) const;

	// TargetActor의 Bounds 기준 판정 위치를 반환하는 함수
	FVector GetTargetCheckLocation(AActor* TargetActor) const;

	// Source BaseDamage와 AttackRow DamageScale 기준 Tick 데미지를 계산하는 함수
	float CalculateFlameDamage(const FNSEnemyAttackRow& AttackRow) const;

	// TargetActor에게 Flame DamageEffect를 적용하는 함수
	bool ApplyFlameDamageToTarget(AActor* TargetActor, const FNSEnemyAttackRow& AttackRow);

	// 현재 공격 대상 Actor를 반환하는 함수
	AActor* ResolveAttackActor() const;

	// AimMode와 타깃 상태 기준으로 화염 조준 위치를 계산하는 함수
	FVector ResolveAimPoint(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// Muzzle 위치와 공격 대상 기준으로 Flame 진행 방향을 계산하는 함수
	FVector ResolveFlameDirection(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// DebugData 기준으로 Flame Cone을 표시하는 함수
	void DrawDebugFlameCone(
		const FNSFlameEmitter& Emitter,
		const FNSEnemyAttackRow& AttackRow) const;

private:
	// Flame Overlap에 사용할 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame")
	TEnumAsByte<ECollisionChannel> FlameTraceChannel = NSCollisionChannels::EnemyWeaponTrace;

	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// Flame Tick 반복 Timer
	FTimerHandle FlameTickTimerHandle;

	// Flame 종료 Timer
	FTimerHandle FlameEndTimerHandle;

protected:
	// 화염 지속 사운드를 재생할 SoundDataTable
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|Sound")
	FName FlameSoundID = FName(TEXT("Monster_TitanWalker_Flame_Loop"));

	// 화염 Niagara를 재생할 VFXDataTable
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX")
	FName FlameVFXID = FName(TEXT("Monster_TitanWalker_Flame_Loop"));

	// Niagara 컴포넌트 자체의 추가 스케일, DT_VFXDataTable ScaleMultiplier와 곱해짐
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.01"))
	float FlameVFXComponentScale = 1.0f;

	// 공격 Range 방향으로 화염 Niagara를 배치할 간격
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "1.0"))
	float FlameVFXForwardSpacing = 180.0f;

	// Cone 좌우 폭 방향으로 화염 Niagara를 배치할 간격
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "1.0"))
	float FlameVFXLateralSpacing = 160.0f;

	// 소켓 하나당 생성 가능한 최대 화염 Niagara 수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "1"))
	int32 MaxFlameVFXPerEmitter = 32;

	// NS_Fire의 개별 불꽃 크기 User Parameter 이름
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX")
	FName FlameScaleParameterName = FName(TEXT("User.Flame Scale"));

	// NS_Fire의 Spawn Rate User Parameter 이름
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX")
	FName FlameSpawnRateParameterName = FName(TEXT("User.Spawn Rate"));

	// NS_Fire의 불꽃 크기 값
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.0"))
	float FlameNiagaraFlameScale = 1.4f;

	// NS_Fire의 Spawn Rate 값
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX", meta = (ClampMin = "0.0"))
	float FlameNiagaraSpawnRate = 80.0f;

	// Niagara 전방 축이 맞지 않을 때 보정하는 회전값
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic|VFX")
	FRotator FlameVFXRotationOffset = FRotator::ZeroRotator;

	// 현재 유지 중인 화염 Niagara 컴포넌트 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveFlameVFXComponents;

	// 화염 지속 사운드 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveFlameAudioComponent;

	// 화염 사운드 재생을 시작하는 함수
	void StartFlameCosmetics();

	// 현재 유효 사거리를 기준으로 화염 VFX 컴포넌트 위치를 갱신하는 함수
	void UpdateFlameVFX(const TArray<FNSFlameEmitter>& Emitters, float EffectiveRange);

	// 현재 유효 사거리 안에서 소켓부터 끝 지점까지 화염 VFX 배치 Transform을 생성하는 함수
	void BuildFlameVFXTransforms(
		const FNSFlameEmitter& Emitter,
		const FNSEnemyAttackRow& AttackRow,
		float EffectiveRange,
		TArray<FTransform>& OutTransforms) const;

	// 유지 중인 화염 Niagara 컴포넌트를 정리하는 함수
	void StopFlameVFXComponents();

	// 화염 사운드와 VFX를 정리하는 함수
	void StopFlameCosmetics();
	
	
	float GetFlameRange(const FNSEnemyAttackRow& AttackRow) const;
	
protected:
	// 화염 유효 사거리가 0에서 AreaData.Range까지 커지는 데 걸리는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame", meta = (ClampMin = "0.0"))
	float FlameRangeGrowDuration = 0.35f;

	// 화염 VFX를 갱신하는 간격. 피해 틱과 분리해서 사거리 성장을 부드럽게 보여줌
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame|VFX", meta = (ClampMin = "0.01"))
	float FlameVFXUpdateInterval = 0.03f;

	// 화염 VFX가 소켓 위치에서 몇 cm 앞부터 배치될지 정함. 소켓에서 바로 나오게 하려면 0
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame|VFX", meta = (ClampMin = "0.0"))
	float FlameVFXStartOffset = 0.0f;

	// 현재 화염 공격이 시작된 월드 시간
	float FlameStartTime = 0.0f;

	// 현재 피해 판정과 VFX에 적용되는 화염 유효 사거리
	float FlameCurrentRange = 0.0f;

	// 화염 VFX만 부드럽게 갱신하기 위한 타이머
	FTimerHandle FlameVFXTimerHandle;

	// 현재 시간 기준으로 0~1 사이의 화염 사거리 성장 비율을 반환하는 함수
	float GetFlameRangeAlpha() const;

	// AreaData.Range를 기준으로 현재 적용해야 할 화염 유효 사거리를 반환하는 함수
	float GetCurrentFlameRange() const;

	// 화염 VFX를 피해 틱과 별도로 갱신하는 함수
	void TickFlameVFX();
};

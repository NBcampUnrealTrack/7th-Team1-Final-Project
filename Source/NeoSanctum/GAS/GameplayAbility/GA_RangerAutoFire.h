// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_RangerAutoFire.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 입력 유지 동안 한 발 단위로 반복 활성화되는 레인저 기본 연사 Ability.
 *
 * 로컬 조작자는 화면 중앙 조준 결과를 TargetData로 생성하고,
 * 서버는 TargetData를 검증한 뒤 Canonical Aim Trace 기준으로 실제 명중과 데미지를 판정한다.
 *
 * 총구가 장애물에 막힌 경우에는 조준 대상보다 총구 앞 장애물을 우선 처리한다.
 */
UCLASS()
class NEOSANCTUM_API UGA_RangerAutoFire : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_RangerAutoFire();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	// AutoFire 데미지 전달에 사용할 GameplayEffect 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 한 발 발사 시 재생할 연출용 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Animation")
	TObjectPtr<UAnimMontage> FireMontage;
	
	// 몽타주 재생 속도. 실제 연사 간격은 FireRate CombatStat이 결정.
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Animation")
	float FireMontagePlayRate = 1.0f;
	
	// 클라이언트 조준 Trace와 서버 Canonical Aim Trace의 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float TraceRange = 10000.0f;
	
	// 클라이언트가 보낸 TraceStart와 서버 카메라 기준 시작점의 최대 허용 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation")
	float ServerTraceStartTolerance = 300.0f;
	
	// 클라이언트 Trace 방향과 서버 Canonical Aim 방향의 최대 허용 각도
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation",
		meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float ServerAimDirectionToleranceDegrees = 15.0f;
	
	// 클라이언트 Hit 위치와 서버 Canonical Trace Hit 위치의 최대 허용 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation")
	float ServerHitLocationTolerance = 200.0f;
	
	// 총구가 벽에 가까운 경우도 감지하도록 총구 막힘 Trace 시작점을 뒤로 보정하는 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation")
	float MuzzleObstructionBackTraceDistance = 100.0f;
	
	// 조준 Trace와 총구 막힘 Trace에 공통으로 사용할 Collision 채널
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TEnumAsByte<ECollisionChannel> TraceChannel = NSCollisionChannels::WeaponTrace;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugHitscan = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugMuzzleObstruction = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineDuration = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineThickness = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineStartOffset = 200.0f;
	
	// 클라이언트 예측키 확인용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bLogPredictionKey = false;
	
private:
	// 현재 활성화된 한 발 발사 주기를 종료.
	void FinishFireCycle();

	// 로컬 화면 중앙 조준 결과로 TargetData를 생성.
	void FireOnce();
	
	void PlayFireMontage();
	
	FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResult(const FHitResult& HitResult) const;
	
	// 클라이언트 또는 서버에서 전달받은 TargetData의 공통 진입점.
	void OnTargetDataReadyCallback(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag);
	
	// 발사 연출, 로컬 예측 피드백, 서버 권한 처리를 역할별로 분기.
	void OnRangerTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	// 서버 Canonical Aim Trace와 총구 막힘 결과를 기준으로 실제 데미지를 적용.
	void ProcessTargetDataForDamage(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	void ApplyDamageToActor(AActor* TargetActor);
	bool TryGetFinalDamage(float& OutDamage);
	bool TryGetFinalFireInterval(float& OutFireInterval);
	void ApplyDamageSetByCaller(FGameplayEffectSpecHandle& InSpecHandle, float InDamage) const;
	
	void ExecuteMuzzleFireCue();
	void ExecuteBulletTrailCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	void ExecuteImpactCue(const FHitResult& HitResult);
	void ExecutePredictedImpactCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	void DrawDebugTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle) const;
	void DrawDebugMuzzleObstructionTrace(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const FHitResult& ObstructionHitResult,
		bool bIsObstructed
	) const;

	// 로컬 소유자에게만 표시해야 하는 예측 피드백인지 확인.
	bool ShouldPlayLocalFeedback() const;
	
	// 로컬 플레이어 화면 중앙 기준으로 조준 TargetData를 생성.
	bool TryBuildHitscanTrace(
		FHitResult& OutHitResult,
		FVector& OutTraceStart,
		FVector& OutTraceEnd,
		bool& bOutHit) const;
	
	// 서버가 보유한 조준 정보로 권한 판정용 Canonical Aim Trace를 생성
	bool TryBuildServerAimTrace(
		FHitResult& OutHitResult,
		FVector& OutTraceStart,
		FVector& OutTraceEnd,
		bool& bOutHit
	) const;
	
	bool TryGetAimTraceStartLocation(FVector& OutLocation) const;
	bool TryGetAttackOriginTransform(FTransform& OutTransform) const;
	
	// 총구와 조준 지점 사이에 조준 대상보다 먼저 맞는 장애물이 있는지 확인.
	bool IsMuzzleObstructed(
		const FVector& AimPoint,
		const AActor* AimTargetActor, 
		FHitResult& OutObstructionHitResult
	) const;
	
	// 클라이언트 TargetData가 서버 Canonical Aim Trace와 일치하는지 검증.
	bool IsTargetDataTraceValid(
		const FHitResult& ClientHitResult,
		const FHitResult& ServerHitResult,
		const FVector& ServerTraceStart,
		const FVector& ServerTraceEnd,
		bool bServerAimHit
	) const;
	
	// AI 청각 감지용 소음 발생
	void ReportWeaponNoise(const AActor* InAvatarActor);
	
	// 데미지 감지 가해자 지정
	void AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle);
	
	// 원격 클라이언트 서버 Ability 종료 판단
	bool IsWaitingForRemoteClientTargetData() const;
	
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
	FTimerHandle FireDelayTimerHandle;
	
	bool bFireCycleElapsed = false;
	bool bTargetDataProcessed = false;
};

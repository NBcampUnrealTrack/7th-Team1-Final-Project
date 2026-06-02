// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_RangerAutoFire.generated.h"

class UGameplayEffect;

/**
 * 원거리 캐릭터 기본 공격
 * 한 번 활성화될 때 한 발을 발사하고, 입력 유지 중 반복 활성화
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
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float FireInterval = 0.15f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float TraceRange = 10000.0f;
	
	// 클라가 보낸 TraceStart가 서버 기준에서 너무 멀면 거부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation")
	float ServerTraceStartTolerance = 300.0f;
	
	// 클라 Hit 위치와 서버 재 Trace Hit 위치가 너무 다르면 거부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation")
	float ServerHitLocationTolerance = 200.0f;
	
	// 총구가 벽에 살짝 파고든 상황까지 잡기 위해 Trace 시작점을 뒤로 당기는 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Validation")
	float MuzzleObstructionBackTraceDistance = 100.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	
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
	void FinishFireCycle();

	void FireOnce();
	
	FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResult(const FHitResult& HitResult) const;
	void OnTargetDataReadyCallback(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag);
	void OnRangerTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	void ProcessTargetDataForDamage(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	void ApplyDamageToActor(AActor* TargetActor);
	
	void ExecuteMuzzleFireCue();
	void ExecuteImpactCue(const FHitResult& HitResult);
	void ExecutePredictedImpactCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	void DrawDebugTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle) const;
	void DrawDebugMuzzleObstructionTrace(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const FHitResult& ObstructionHitResult,
		bool bIsObstructed
	) const;

	// 로컬 조작자인지 판단
	bool ShouldPlayLocalFeedback() const;
	bool TryBuildHitscanTrace(
		FHitResult& OutHitResult,
		FVector& OutTraceStart,
		FVector& OutTraceEnd,
		bool& bOutHit) const;
	
	bool TryGetAimTraceStartLocation(FVector& OutLocation) const;
	bool TryGetAttackOriginTransform(FTransform& OutTransform) const;
	
	bool IsMuzzleObstructed(const FHitResult& ServerHitResult, FHitResult& OutObstructionHitResult) const;
	bool ValidateTargetDataHitResult(const FHitResult& ClientHitResult, FHitResult& OutServerHitResult) const;
	
	// AI 청각 감지용 소음 발생
	void ReportWeaponNoise(const AActor* InAvatarActor);
	
	// AI 데미지 감지용 가해자 지정
	void AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle);
	
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
	FTimerHandle FireDelayTimerHandle;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_EngineerShotgunFire.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 엔지니어 샷건 발사 Ability
 * RangerAutoFire의 예측/검증 흐름을 기반으로 제작. 
 * 한 번의 발사에서 여러 펠릿 Trace를 생성하는 형태로 추가
 */
UCLASS()
class NEOSANCTUM_API UGA_EngineerShotgunFire : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_EngineerShotgunFire();

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
	
private:
	// 한 번의 샷건 발사 수행 + 펠릿 TargetData 제작
	void FireOnce();
	
	// 발사 연출용 몽타주 재생
	void PlayFireMontage();
	
	// FirerInterval 계산에 따라 발사 대기시간을 적용해서 중복사용시 Ability 종료를 시도
	void FinishFireCycle();
	
private:
	// 최종 대미지 스탯 조회. 현재 펠릿 하나마다 데미지를 적용하는 방식.
	bool TryGetFinalDamage(float& OutDamage);
	
	// 최종 FireRate 스탯을 FireInterval로 변환
	bool TryGetFinalFireInterval(float& OutFireInterval);
	
	// 최종 FireRange 스탯 조회
	bool TryGetFinalFireRange(float& OutFireRange) const;
	
	// 최종 PelletCount 스탯 조회
	bool TryGetFinalPelletCount(int32& OutPelletCount) const;
	
	// 최종 Spread 스탯을 조회
	bool TryGetFinalSpreadAngleDegrees(float& OutSpreadAngleDegrees) const;
	
private:
	// 조준 시작 위치를 캐릭터로부터 가져오기
	bool TryGetAimTraceStartLocation(FVector& OutLocation) const;

	// 현재 무기의 총구 Transform 가져오기
	bool TryGetAttackOriginTransform(FTransform& OutTransform) const;
	
	// 화면 중앙 조준 기준의 Trace 시작점과 중심 방향 만들기
	bool TryBuildShotgunTraceBasis(FVector& OutTraceStart, FVector& OutCenterDirection) const;

	// 중심 방향에서 산탄각을 반영한 펠릿 방향을 계산해서 만들어냄
	FVector BuildPelletDirection(
		const FVector& CenterDirection,
		int32 PelletIndex,
		float FinalSpreadAngleDegrees
	) const;

	// 단일 펠릿 LineTrace
	bool TryBuildPelletTrace(
		const FVector& TraceStart,
		const FVector& TraceDirection,
		float FinalFireRange,
		FHitResult& OutHitResult,
		FVector& OutTraceEnd,
		bool& bOutHit
	) const;

	// 여러 HitResult를 서버로 전달 가능한 TargetDataHandle로 변환
	FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResults(const TArray<FHitResult>& HitResults) const;

	// TargetData가 준비되면 서버 전송, 로컬 피드백, 서버 대미지 처리를 순차적으로 진행
	void OnTargetDataReadyCallback(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle,
		FGameplayTag ApplicationTag
	);

	// TargetData를 받은 뒤 Cue와 서버 전용 처리를 분기
	void OnShotgunTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	// 서버에서 펠릿별 TargetData를 검증하고 대미지를 적용
	void ProcessTargetDataForDamage(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	// 대상 Actor에게 GameplayEffect 대미지를 적용
	void ApplyDamageToActor(AActor* TargetActor);

	// Damage GameplayEffect의 SetByCaller 값을 설정
	void ApplyDamageSetByCaller(FGameplayEffectSpecHandle& InSpecHandle, float InDamage) const;

	// 총구 발사 GameplayCue를 실행
	void ExecuteMuzzleFireCue();

	// 펠릿별 BulletTrail GameplayCue를 실행
	void ExecuteBulletTrailCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	// Impact GameplayCue를 실행
	void ExecuteImpactCue(const FHitResult& HitResult);

	// 로컬 예측용 Impact GameplayCue를 실행
	void ExecutePredictedImpactCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	// 로컬 조작자인지 판단
	bool ShouldPlayLocalFeedback() const;

	// 총구와 조준 지점 사이가 다른 물체에 막혔는지 확인
	bool IsMuzzleObstructed(
		const FVector& AimPoint,
		const AActor* AimTargetActor,
		FHitResult& OutObstructionHitResult
	) const;

	// 서버 권한 판정용 펠릿 Trace를 생성
	bool TryBuildServerPelletTrace(
		const FHitResult& ClientHitResult,
		FHitResult& OutHitResult,
		FVector& OutTraceStart,
		FVector& OutTraceEnd,
		bool& bOutHit
	) const;

	// 클라이언트 TargetData가 서버 Trace와 일치하는지 검증
	bool IsTargetDataTraceValid(
		const FHitResult& ClientHitResult,
		const FHitResult& ServerHitResult,
		const FVector& ServerTraceStart,
		const FVector& ServerTraceEnd,
		bool bServerAimHit
	) const;

	// 데미지 감지 가해자 지정
	void AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle);

	// 원격 클라이언트 서버 Ability 종료 판단
	bool IsWaitingForRemoteClientTargetData() const;

private:
	// TargetData 기반 Trace 디버그 라인
	void DrawDebugTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle) const;

	// 총구 막힘 검사 디버그 라인
	void DrawDebugMuzzleObstructionTrace(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const FHitResult& ObstructionHitResult,
		bool bIsObstructed
	) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun|Animation")
	float FireMontagePlayRate = 1.0f;

	// 클라가 보낸 TraceStart가 서버 기준에서 너무 멀면 거부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun|Validation")
	float ServerTraceStartTolerance = 300.0f;

	// 클라 Hit 위치와 서버 재 Trace Hit 위치가 너무 다르면 거부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun|Validation")
	float ServerHitLocationTolerance = 200.0f;

	// 총구가 벽에 살짝 파고든 상황까지 잡기 위해 Trace 시작점을 뒤로 당기는 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun|Validation")
	float MuzzleObstructionBackTraceDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Engineer|Shotgun")
	TEnumAsByte<ECollisionChannel> TraceChannel = NSCollisionChannels::PlayerWeaponTrace;
	
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
	
	/// 클라이언트 예측키 확인용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bLogPredictionKey = false;

private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
	FTimerHandle FireDelayTimerHandle;

	bool bFireCycleElapsed = false;
	bool bTargetDataProcessed = false;
};

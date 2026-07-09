// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_Parkour.generated.h"

class ACharacter;
class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

struct FNSParkourTarget
{
	FVector ActorLocation = FVector::ZeroVector;
	FRotator ActorRotation = FRotator::ZeroRotator;
	FVector SurfaceLocation = FVector::ZeroVector;
	float ObstacleHeight = 0.0f;
};

UCLASS()
class NEOSANCTUM_API UGA_Parkour : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Parkour();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

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
	// 파쿠르 이동 Task 종료 처리
	UFUNCTION()
	void OnParkourMoveFinished();

	// 몽타주 정상 종료 처리
	UFUNCTION()
	void OnParkourMontageCompleted();

	// 몽타주 중단 처리
	UFUNCTION()
	void OnParkourMontageInterrupted();

	// Vault 가능 지점 탐색
	bool TryFindParkourTarget(const ACharacter* Character, FNSParkourTarget& OutTarget) const;
	// 파쿠르 진행 방향 계산
	bool TryGetParkourForwardDirection(const ACharacter* Character, FVector& OutDirection) const;
	// 파쿠르 중 상태 태그 부여
	void AddParkourStateTags();
	// 파쿠르 상태 태그 제거
	void RemoveParkourStateTags();
	// MovementMode 복구
	void RestoreMovementMode();
	// Motion Warping 타겟 갱신
	void UpdateMotionWarpTarget(const FNSParkourTarget& Target) const;
	// Vault 몽타주 재생
	bool PlayParkourMontage(const FNSParkourTarget& Target);
	// 착지 지점까지 이동 시작
	bool StartParkourMove(const FNSParkourTarget& Target);
	// 재생할 몽타주 섹션 선택
	FName SelectParkourMontageSection() const;

private:
	// Vault 재생 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Montage")
	TObjectPtr<UAnimMontage> ParkourMontage;

	// Vault 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Montage", meta = (ClampMin = "0.01"))
	float ParkourMontagePlayRate = 1.0f;

	// 랜덤 재생 섹션 목록 : 비어 있으면 첫 섹션부터 재생
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Montage")
	TArray<FName> ParkourMontageSectionNames = { TEXT("Vault_1"), TEXT("Vault_2"), TEXT("Vault_3"), TEXT("Vault_4") };

	// 착지 지점까지 이동 시간
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Movement", meta = (ClampMin = "0.01"))
	float MoveDuration = 0.35f;

	// 전방 장애물 탐색 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float ForwardTraceDistance = 180.0f;

	// 벽면 탐색 높이 : 바닥 기준
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace")
	float WallTraceHeight = 45.0f;

	// Vault 가능 최소 높이
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float MinObstacleHeight = 35.0f;

	// Vault 가능 최대 높이
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float MaxObstacleHeight = 130.0f;

	// 상단 탐색 추가 높이
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float TopTraceExtraHeight = 80.0f;

	// 상단 탐색 전방 보정
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float TopTraceForwardOffset = 45.0f;

	// 착지 후보 전방 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float LandingForwardOffset = 110.0f;

	// 착지 탐색 시작 높이
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float LandingTraceUpOffset = 120.0f;

	// 착지 탐색 하강 거리
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0"))
	float LandingTraceDownDistance = 220.0f;

	// 벽면 정면 판정값
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinWallFacingDot = 0.35f;

	// 바닥 판정 Normal Z
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinLandingNormalZ = 0.65f;

	// Vault 판정 Trace 채널
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 디버그 Trace 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Debug")
	bool bDrawDebugTrace = false;

	// 디버그 Trace 유지 시간
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|Debug", meta = (ClampMin = "0.0"))
	float DebugTraceDuration = 2.0f;

	// Motion Warping 타겟 이름
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|MotionWarping")
	FName MotionWarpTargetName = TEXT("ParkourTarget");

	// Motion Warping 사용 여부
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Parkour|MotionWarping")
	bool bEnableMotionWarping = true;

	// 착지 이동 Task
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> MoveTask;

	// 몽타주 재생 Task
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	// MovementMode 복구 캐시
	TOptional<TEnumAsByte<EMovementMode>> PreviousMovementMode;
};

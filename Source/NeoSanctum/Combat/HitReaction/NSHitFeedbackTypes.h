// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSHitFeedbackTypes.generated.h"

// 히트 피드백 대상 분류
UENUM(BlueprintType)
enum class ENSHitFeedbackTargetType : uint8
{
	Any,
	Player,
	Enemy,
	Barrier,
	DestructibleObject,
	Turret
};

// 히트 판정 품질 : 추후에 추가될 사항까지 고려했으나 아직 Critical, Headshot은 로직이 없음
UENUM(BlueprintType)
enum class ENSHitFeedbackQuality : uint8
{
	Any,
	Normal,
	Critical,
	HeadShot
};

// 히트에 따른 결과
UENUM(BlueprintType)
enum class ENSHitFeedbackOutcome : uint8
{
	Any,
	None,
	Destroy,
	Kill
};

// 데미지가 적용된 레이어 분류
UENUM(BlueprintType)
enum class ENSHitReactionDamageLayer : uint8
{
	Any,
	Health,
	Shield
};

// 공격 방식 분류
UENUM(BlueprintType)
enum class ENSHitReactionAttackType : uint8
{
	Any,
	Ranged,
	Melee
};

// 크로스헤어 공격 피드백 종류 : 추후에 추가될 사항까지 고려했으나 아직 Critical, Headshot은 로직이 없음
UENUM(BlueprintType)
enum class ENSCrosshairAttackFeedbackType : uint8
{
	None,
	NormalAttack,
	CriticalAttack,
	HeadShot,
	ShieldBarrierAttack,
	Kill,
	Destroy
};

// 플레이어가 피해를 받았을 때의 로컬 피드백 종류
UENUM(BlueprintType)
enum class ENSHitTakenFeedbackType : uint8
{
	None,
	ShieldHit,
	ShieldBroken,
	HealthHit
};

// 플레이어 피격 피드백의 상태성 연출 종류
UENUM(BlueprintType)
enum class ENSHitTakenFeedbackStateType : uint8
{
	None,
	ShieldRecharging
};

// 히트 피드백 판정에 필요한 공통 Context
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitFeedbackContext
{
	GENERATED_BODY()
	
	// 타겟
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	ENSHitFeedbackTargetType TargetType = ENSHitFeedbackTargetType::Any;
	
	// 히트가 어떤 타입인지 : Normal, Critical, Headshot 구분용
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	ENSHitFeedbackQuality HitQuality = ENSHitFeedbackQuality::Normal;
	
	// 히트의 결과 : Hit결가로 Kill이나 Destroy가 되는 경우
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	ENSHitFeedbackOutcome Outcome = ENSHitFeedbackOutcome::None;
	
	// 타겟 액터
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	TObjectPtr<AActor> TargetActor = nullptr;
	
	// 히트 지점
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	FVector HitLocation = FVector::ZeroVector;
	
	// 타겟이 죽었는지를 판단
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	bool bTargetDead = false;
};

// GMS로 크로스헤어 공격 피드백을 전달할 때 사용하는 메시지
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSCrosshairAttackFeedbackMessage
{
	GENERATED_BODY()
	
	// 어떤 크로스헤어 피드백을 재생할지
	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	ENSCrosshairAttackFeedbackType FeedbackType = ENSCrosshairAttackFeedbackType::None;
	
	// HitFeedback 관련 정보 구조체
	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	FNSHitFeedbackContext Context;
};

// 플레이어 피격 피드백에 필요한 Context
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitTakenFeedbackContext
{
	GENERATED_BODY()
	
	// 플레이어가 피해를 받았을 때의 로컬 피드백 종류
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	ENSHitTakenFeedbackType FeedbackType = ENSHitTakenFeedbackType::None;
	
	// 얼마나 데미지를 받았는지 (나중에 로그나 UI 표시용)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	float DamageAmount = 0.0f;
	
	// 히트 지점 (파티클 표시용)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	FVector HitLocation = FVector::ZeroVector;
	
	// 공격자 위치 (UI 상의 방향 표시용)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	FVector InstigatorLocation = FVector::ZeroVector;
	
	// 현재 쉴드 비율
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	float ShieldRatio = 0.0f;
	
	// 현재 체력 비율
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitFeedback")
	float HealthRatio = 0.0f;
};

// GMS로 플레이어 피격 피드백을 전달할 때 사용하는 메시지
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitTakenFeedbackMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	FNSHitTakenFeedbackContext Context;
};

// GMS로 플레이어 피격 상태성 피드백을 전달할 때 사용하는 메시지
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitTakenFeedbackStateMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	ENSHitTakenFeedbackStateType StateType = ENSHitTakenFeedbackStateType::None;

	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	bool bActive = false;
};

// GMS로 플레이어 피격 관련 생존 비율을 전달할 때 사용하는 메시지
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitTakenFeedbackVitalsMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	float HealthRatio = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HitFeedback")
	float ShieldRatio = 1.0f;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UPoseSearchDatabase;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

UENUM(BlueprintType)
enum class ENSAnimState : uint8
{
	// 기본 이동 상태
	Idle,
	Walk,
	Run,
	Sprint,

	// 기본 이동 상태가 바뀌는 순간의 전환 상태
	WalkToRun,
	WalkToSprint,
	RunToWalk,
	RunToSprint,
	SprintToWalk,
	SprintToRun,

	// 공중/착지 상태
	JumpStart,
	FallLoop,
	LandLight,
	LandHeavy
};

UCLASS()
class NEOSANCTUM_API UNSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Animation|State", meta = (BlueprintThreadSafe))
	void SetSelectedAnimState(ENSAnimState NewSelectedAnimState);

	UFUNCTION(BlueprintCallable, Category = "Animation|State", meta = (BlueprintThreadSafe))
	void SetSelectedPoseSearchDatabase(UPoseSearchDatabase* NewSelectedPoseSearchDatabase);

protected:
	// 소유 캐릭터와 애니메이션에서 읽을 컴포넌트를 캐싱
	void RefreshOwningCharacter();

	// CharacterMovement 값을 애니메이션용 변수로 복사하고 Locomotion 상태를 갱신
	void UpdateMovementData(float DeltaSeconds);

	// 현재 이동/공중/착지 조건을 Chooser Table에서 사용할 상태로 정리
	void UpdateAnimState();

protected:
	// 캐릭터 / 컴포넌트 캐시
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	// 이동 입력 / 속도 값
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	FVector LocalVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float VerticalVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bWasFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsMovingUp = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bHasLandRequest = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsHeavyLand = false;

	// Locomotion 상태
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState PreviousGroundedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState CurrentGroundedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState AnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState SelectedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float LandVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float PreviousVerticalVelocity = 0.f;

	// 이동 상태 기본값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float HeavyLandVelocityThreshold = 900.f;
};

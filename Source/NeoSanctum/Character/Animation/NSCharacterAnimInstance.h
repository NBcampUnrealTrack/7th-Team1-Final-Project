// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UPoseSearchDatabase;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

// Chooser Table이 읽는 최종 locomotion 상태입니다.
// 지상 이동도 Idle/Walk/Run/Sprint로 나누어 PSD를 세분화할 수 있게 합니다.
UENUM(BlueprintType)
enum class ENSAnimState : uint8
{
	Idle,
	Walk,
	Run,
	Sprint,
	WalkToRun,
	WalkToSprint,
	RunToWalk,
	RunToSprint,
	SprintToWalk,
	SprintToRun,
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
	void RefreshOwningCharacter();
	void UpdateMovementData(float DeltaSeconds);
	void UpdateAnimState();

protected:
	// 소유 캐릭터와 공통 컴포넌트 캐시입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	// CharacterMovement에서 가져온 현재 이동 값입니다.
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

	// 착지 프레임에 한 번 발생하는 Land DB 선택 요청입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bHasLandRequest = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsHeavyLand = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState PreviousGroundedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState CurrentGroundedAnimState = ENSAnimState::Idle;

	// Chooser Table이 읽는 상태입니다. 이 값으로 검색할 Pose Search Database를 고릅니다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState AnimState = ENSAnimState::Idle;

	// OnUpdate에서 Chooser가 선택한 상태/DB를 디버깅하고 후속 확장에 활용하기 위한 캐시입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState SelectedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	// 착지 직전 수직 속도를 기반으로 Light/Heavy Land를 나누기 위한 값입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float LandVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float PreviousVerticalVelocity = 0.f;

	// 지상 이동 상태를 속도 기반으로 나누기 위한 기준값입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float HeavyLandVelocityThreshold = 900.f;
};

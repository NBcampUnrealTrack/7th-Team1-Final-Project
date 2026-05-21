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
	// 기존 Chooser Table 호환용 통합 상태
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

UENUM(BlueprintType)
enum class ENSLocomotionState : uint8
{
	// 지상 이동 상태
	Idle,
	Walk,
	Run,
	Sprint,

	// 속도 전환 상태
	WalkToRun,
	WalkToSprint,
	RunToWalk,
	RunToSprint,
	SprintToWalk,
	SprintToRun
};

UENUM(BlueprintType)
enum class ENSAirState : uint8
{
	// 공중/착지 상태
	Grounded,
	JumpStart,
	FallLoop,
	LandLight,
	LandHeavy
};

UENUM(BlueprintType)
enum class ENSTurnInPlaceState : uint8
{
	// 제자리 회전 상태
	None,
	Left90,
	Right90,
	Left180,
	Right180
};

UENUM(BlueprintType)
enum class ENSUpperBodyState : uint8
{
	// 상체 레이어 상태
	None,
	Aim,
	Fire,
	Reload
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

	UFUNCTION(BlueprintCallable, Category = "Animation|State", meta = (BlueprintThreadSafe))
	void SetUpperBodyState(ENSUpperBodyState NewUpperBodyState);

protected:
	// 캐릭터와 컴포넌트를 캐싱
	void RefreshOwningCharacter();

	// CharacterMovement 값을 애니메이션 변수로 복사
	void UpdateMovementData(float DeltaSeconds);

	// 이동 상태를 계산
	void UpdateLocomotionState();

	// 공중/착지 상태를 계산
	void UpdateAirState();

	// 제자리 회전 상태를 계산
	void UpdateTurnInPlaceState();

	// AimOffset 입력값을 계산
	void UpdateAimData(float DeltaSeconds);

	// 기존 Chooser Table이 읽는 상태를 갱신
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

	// Locomotion / Air / Turn 상태
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSLocomotionState PreviousLocomotionState = ENSLocomotionState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSLocomotionState LocomotionState = ENSLocomotionState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAirState AirState = ENSAirState::Grounded;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	ENSTurnInPlaceState TurnInPlaceState = ENSTurnInPlaceState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	bool bIsTurnInPlaceActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceYawDelta = 0.f;

	// Chooser Table용 상태
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

	// Aim / 상체 상태
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimOffsetAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Upper Body")
	ENSUpperBodyState UpperBodyState = ENSUpperBodyState::None;

	// 이동 상태 판정 기준값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float HeavyLandVelocityThreshold = 900.f;

	// Turn In Place 판정 기준값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStartAngle = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStopAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlace180Angle = 135.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AimYawLimit = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AimPitchLimit = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim", meta = (ClampMin = "0.0"))
	float AimOffsetBlendSpeed = 10.f;
};

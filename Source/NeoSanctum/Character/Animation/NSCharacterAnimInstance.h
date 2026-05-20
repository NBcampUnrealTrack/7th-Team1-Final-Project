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
enum class ENSTurnInPlaceDirection : uint8
{
	None,
	Left,
	Right
};

UENUM(BlueprintType)
enum class ENSCharacterAnimType : uint8
{
	Melee,
	Ranged
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
	void UpdateAimData();
	void UpdateTurnInPlaceData();

protected:
	// Cached owner and shared components.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	// Movement values copied from CharacterMovement.
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

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState PreviousGroundedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState CurrentGroundedAnimState = ENSAnimState::Idle;

	// Main state read by Chooser Tables.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState AnimState = ENSAnimState::Idle;

	// Cached Chooser result for debugging and later selection-aware transitions.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState SelectedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float LandVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float PreviousVerticalVelocity = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float HeavyLandVelocityThreshold = 900.f;

	// Character type controls how strongly the shared AimOffset is applied in the AnimBP.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	ENSCharacterAnimType CharacterAnimType = ENSCharacterAnimType::Melee;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float MeleeAimOffsetAlpha = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float RangedAimOffsetAlpha = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimOffsetAlpha = 0.4f;

	// Shared upper/lower-body separation and turn-in-place values.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float ViewYawDelta = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	bool bShouldTurnInPlace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	ENSTurnInPlaceDirection TurnInPlaceDirection = ENSTurnInPlaceDirection::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStartAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStopAngle = 10.f;
};

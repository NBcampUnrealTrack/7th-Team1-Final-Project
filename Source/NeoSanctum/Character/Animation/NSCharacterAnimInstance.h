// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

UENUM(BlueprintType)
enum class ENSAnimMovementMode : uint8
{
	OnGround,
	InAir
};

UENUM(BlueprintType)
enum class ENSAnimMovementState : uint8
{
	Idle,
	Moving
};

UENUM(BlueprintType)
enum class ENSAnimGait : uint8
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class ENSAnimCombatType : uint8
{
	None,
	Ranged,
	Melee
};

UENUM(BlueprintType)
enum class ENSTurnInPlaceDirection : uint8
{
	None,
	Left90,
	Right90,
	Left180,
	Right180
};

UCLASS()
class NEOSANCTUM_API UNSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatType(ENSAnimCombatType NewCombatType);

protected:
	void RefreshCachedReferences();
	void ResetRuntimeData();

	void UpdateMovementData();
	void UpdateMovementMode();
	void UpdateMovementState();
	void UpdateGait();
	void UpdateStartStopData(float DeltaSeconds);
	void UpdateLandingData(float DeltaSeconds);
	void UpdatePivotData(float DeltaSeconds);
	void UpdateSpinTransitionData();
	void UpdateCombatData(float DeltaSeconds);
	void UpdateAimData();
	void UpdateTurnInPlaceData();
	void UpdateTimeToLand();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector LocalVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector LocalAcceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float Speed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float PreviousSpeed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float StopSpeed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float VerticalVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float LocomotionAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float AccelerationAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimMovementMode MovementMode = ENSAnimMovementMode::OnGround;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimMovementState MovementState = ENSAnimMovementState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimMovementState PreviousMovementState = ENSAnimMovementState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimGait Gait = ENSAnimGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimGait PreviousGait = ENSAnimGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimGait StopGait = ENSAnimGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bIsStarting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bIsPivoting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bJustLandedLight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bJustLandedHeavy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bShouldTurnInPlace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bShouldSpinTransition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	float TimeToLand = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Combat")
	ENSAnimCombatType CombatType = ENSAnimCombatType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Combat")
	bool bUseUpperBodyLayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	ENSTurnInPlaceDirection TurnInPlaceDirection = ENSTurnInPlaceDirection::None;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceYawDelta = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float MoveSpeedThreshold = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float HeavyLandSpeedThreshold = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float LandStateHoldTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float StartStateHoldTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float PivotStateHoldTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float PivotAccelerationDotThreshold = -0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SpinTransitionAngle = 135.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float TimeToLandTraceDistance = 5000.f;

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

private:
	bool bWasFalling = false;
	float PreviousVerticalVelocity = 0.f;
	float StartStateRemainingTime = 0.f;
	float LandStateRemainingTime = 0.f;
	float PivotStateRemainingTime = 0.f;
};

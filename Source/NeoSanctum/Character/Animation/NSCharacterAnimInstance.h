// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

UENUM(BlueprintType)
enum class ENSAnimGait : uint8
{
	Walk,
	Run,
	Sprint
};

UCLASS()
class NEOSANCTUM_API UNSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	void RefreshOwningCharacter();
	void UpdateMovementData(float DeltaSeconds);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

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
	bool bJustLanded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimGait Gait = ENSAnimGait::Run;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float JustLandedDuration = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float TimeSinceLanded = 999.f;
};

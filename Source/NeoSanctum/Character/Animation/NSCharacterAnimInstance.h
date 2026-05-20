// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UPoseSearchDatabase;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

// 이동 속도 Enum
UENUM(BlueprintType)
enum class ENSAnimGait : uint8
{
	Walk,
	Run,
	Sprint
};

// Chooser Table이 읽는 최종 locomotion 상태
UENUM(BlueprintType)
enum class ENSAnimState : uint8
{
	Grounded,
	Sprint,
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
	// 캐릭터 및 컴포넌트 캐싱
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	// Character Movement 기반 변수들
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
	ENSAnimGait Gait = ENSAnimGait::Run;

	// Chooser Table에서 읽게되는 상태
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState AnimState = ENSAnimState::Grounded;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState SelectedAnimState = ENSAnimState::Grounded;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	// 착지 직전 속도를 기반으로 Light/Heavy Land를 나누는 변수들
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float LandVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float PreviousVerticalVelocity = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float HeavyLandVelocityThreshold = 900.f;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSEnemyAnimInstance.generated.h"

class ANSEnemyCharacterBase;
class UCharacterMovementComponent;

UCLASS()
class NEOSANCTUM_API UNSEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	void UpdateLeftHandIK(float DeltaSeconds);
	void UpdateLeftHandIKAlpha(float TargetAlpha, float DeltaSeconds);
	void UpdateAimRotation(float DeltaSeconds);

protected:
	// 이동 속도
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;

	// 이동 여부
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;
	
	// 체공 여부
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	// 상승/낙하 구분을 위한 Z축 속도
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float VerticalVelocity = 0.0f;

	// 이동 기준 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Config")
	float MovingSpeedThreshold = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Hand IK")
	float LeftHandIKAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Hand IK")
	FTransform LeftHandIKTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hand IK", meta = (ClampMin = "0.0"))
	float LeftHandIKInterpSpeed = 12.0f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimAlpha = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float AimInterpSpeed = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float MaxAimPitch = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float MaxAimYaw = 60.0f;

private:
	UPROPERTY()
	TObjectPtr<ANSEnemyCharacterBase> EnemyCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
};

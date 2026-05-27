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
	// 이동 속도
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;

	// 이동 여부
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;

	// 이동 기준 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Config")
	float MovingSpeedThreshold = 3.0f;

private:
	UPROPERTY()
	TObjectPtr<ANSEnemyCharacterBase> EnemyCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
};

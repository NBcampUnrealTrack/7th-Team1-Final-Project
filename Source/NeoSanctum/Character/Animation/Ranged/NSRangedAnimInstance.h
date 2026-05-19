// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Character/Animation/NSCharacterAnimInstance.h"
#include "NSRangedAnimInstance.generated.h"

UENUM(BlueprintType)
enum class ENSTurnInPlaceDirection : uint8
{
	None,
	Left,
	Right
};

UCLASS()
class NEOSANCTUM_API UNSRangedAnimInstance : public UNSCharacterAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Animation|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Animation|Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Animation|Aim")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Animation|Turn In Place")
	bool bShouldTurnInPlace = false;

	UPROPERTY(BlueprintReadWrite, Category = "Animation|Turn In Place")
	ENSTurnInPlaceDirection TurnInPlaceDirection = ENSTurnInPlaceDirection::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStartAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStopAngle = 10.f;
};

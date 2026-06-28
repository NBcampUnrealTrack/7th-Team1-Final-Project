// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NSGameplayCueNotify_BulletTrail.generated.h"


/**
 * BulletTrail GameplayCue.
 * Uses Parameters.Location as start and Parameters.Normal * Parameters.RawMagnitude as end offset.
 */
UCLASS(Blueprintable)
class NEOSANCTUM_API UNSGameplayCueNotify_BulletTrail : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) const override;

protected:
	// BulletTrail 나이아가라 시스템
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|BulletTrail")
	FName BulletTrailVFXID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|BulletTrail", meta = (ClampMin = "0.0"))
	float VFXScaleMultiplier = 1.0f;
	
	// Trail의 시작지점 User Parameter
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|BulletTrail")
	FName StartParameterName = TEXT("User.Start");
	
	// Trail의 끝나는 지점 User Parameter
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|BulletTrail")
	FName EndParameterName = TEXT("User.End");
};

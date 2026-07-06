// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBarrierBase.h"
#include "NSVanguardGuardBarrier.generated.h"

class UBoxComponent;

UCLASS()
class NEOSANCTUM_API ANSVanguardGuardBarrier : public ANSBarrierBase
{
	GENERATED_BODY()

public:
	ANSVanguardGuardBarrier();

protected:
	virtual void ApplyCollisionRadius(float Radius) override;
	virtual void ApplyVisualRadius(float Radius) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UBoxComponent> BoxBarrierCollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|VanguardGuard", meta = (ClampMin = "0.01"))
	float BoxDepthRatio = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|VanguardGuard", meta = (ClampMin = "0.01"))
	float BoxWidthRatio = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|VanguardGuard", meta = (ClampMin = "0.01"))
	float BoxHeightRatio = 0.75f;
};

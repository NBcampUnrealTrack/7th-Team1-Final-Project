// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBarrierBase.h"
#include "NSEngineerBarrier.generated.h"

class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSEngineerBarrier : public ANSBarrierBase
{
	GENERATED_BODY()

public:
	ANSEngineerBarrier();

protected:
	virtual void ApplyCollisionRadius(float Radius) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<USphereComponent> SphereBarrierCollisionComponent;
};

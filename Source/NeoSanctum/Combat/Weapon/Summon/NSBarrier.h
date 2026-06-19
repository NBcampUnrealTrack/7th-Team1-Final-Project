// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSBarrier.generated.h"

UCLASS()
class NEOSANCTUM_API ANSBarrier : public AActor
{
	GENERATED_BODY()

public:
	ANSBarrier();

protected:
	virtual void BeginPlay() override;
};

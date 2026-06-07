// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSTurret.generated.h"

UCLASS()
class NEOSANCTUM_API ANSTurret : public AActor
{
	GENERATED_BODY()

public:
	ANSTurret();

protected:
	virtual void BeginPlay() override;
};

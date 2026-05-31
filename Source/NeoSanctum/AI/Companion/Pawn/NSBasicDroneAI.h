// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NSBasicDroneAI.generated.h"

UCLASS()
class NEOSANCTUM_API ANSBasicDroneAI : public ANSBaseCompanionAI
{
	GENERATED_BODY()

public:
	ANSBasicDroneAI();

protected:
	virtual void BeginPlay() override;
};

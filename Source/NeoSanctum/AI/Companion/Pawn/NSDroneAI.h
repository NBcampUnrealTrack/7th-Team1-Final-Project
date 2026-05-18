// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NSDroneAI.generated.h"

UCLASS()
class NEOSANCTUM_API ANSDroneAI : public ANSBaseCompanionAI
{
	GENERATED_BODY()

public:
	ANSDroneAI();
	
protected:
	virtual void BeginPlay() override;
	
};

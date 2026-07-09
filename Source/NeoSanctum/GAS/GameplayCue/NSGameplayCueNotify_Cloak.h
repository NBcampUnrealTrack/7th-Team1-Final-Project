// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueNotify_Sustainable.h"
#include "NSGameplayCueNotify_Cloak.generated.h"

UCLASS(Blueprintable)
class NEOSANCTUM_API ANSGameplayCueNotify_Cloak : public ANSGameplayCueNotify_Sustainable
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) override;

	virtual bool OnRemove_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) override;
};
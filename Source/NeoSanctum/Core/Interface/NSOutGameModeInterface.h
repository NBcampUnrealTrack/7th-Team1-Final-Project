// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSOutGameModeInterface.generated.h"



UINTERFACE(MinimalAPI, Blueprintable)
class UNSOutGameInterface : public UInterface { GENERATED_BODY() };

class NEOSANCTUM_API INSOutGameInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "GameFlow")
	void RequestStartRun();
};

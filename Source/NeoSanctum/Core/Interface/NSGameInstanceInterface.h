// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSGameInstanceInterface.generated.h"

class UNSLevelCatalog;

UINTERFACE(MinimalAPI, Blueprintable)
class UNSGameInstanceInterface : public UInterface
{
	GENERATED_BODY()
};

class NEOSANCTUM_API INSGameInstanceInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HideLoadingScreen();
	
	virtual UNSLevelCatalog* GetLevelCatalog() const = 0;
};

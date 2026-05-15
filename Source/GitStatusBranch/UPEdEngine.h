// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdEngine.h"
#include "UPEdEngine.generated.h"

/**
 * 
 */
UCLASS()
class GITSTATUSBRANCH_API UUPEdEngine : public UUnrealEdEngine
{
	GENERATED_BODY()
	
	virtual void Init(IEngineLoop* InEngineLoop) override;
};

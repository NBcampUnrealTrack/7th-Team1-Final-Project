// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NSOutGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnReadyStateChanged);

UCLASS()
class NEOSANCTUM_API ANSOutGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	bool IsAllPlayersReady() const;
	
	UPROPERTY(BlueprintAssignable, Category = "Ready")
	FNSOnReadyStateChanged OnReadyStateChanged;

	void NotifyReadyStateChanged();
};

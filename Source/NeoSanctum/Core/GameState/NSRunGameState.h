// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NSRunGameState.generated.h"

class UNSRunGameDataComponent;

/**
 *
 */
UCLASS()
class NEOSANCTUM_API ANSRunGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANSRunGameState();

	UFUNCTION(BlueprintPure, Category = "NS|RunGameData")
	UNSRunGameDataComponent* GetRunGameDataComponent() const { return RunGameDataComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NS|RunGameData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSRunGameDataComponent> RunGameDataComponent;
};

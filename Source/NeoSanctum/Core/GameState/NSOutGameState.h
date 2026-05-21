// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NSOutGameState.generated.h"

class UNSOutGameDataComponent;

UCLASS()
class NEOSANCTUM_API ANSOutGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANSOutGameState();

	bool IsAllPlayersReady() const;

	UFUNCTION(BlueprintPure, Category = "NS|OutGameData")
	UNSOutGameDataComponent* GetOutGameDataComponent() const { return OutGameDataComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NS|OutGameData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSOutGameDataComponent> OutGameDataComponent;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NSPermanentSaveGame.generated.h"

UCLASS()
class NEOSANCTUM_API UNSPermanentSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	TSet<FName> UnlockedNPCIds;

	UPROPERTY(SaveGame)
	int64 TotalCurrency = 0;

	UPROPERTY(SaveGame)
	TArray<FName> EquippedPartIds;

};

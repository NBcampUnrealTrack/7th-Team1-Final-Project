// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NSPermanentSaveGame.generated.h"

// 캐릭터의 영구 저장 데이터
USTRUCT(BlueprintType)
struct FNSCharacterSaveData
{
	GENERATED_BODY()

	// 총 재화
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int64 TotalCurrency = 0;

	// 장비한 파츠 Ids
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FName> EquippedPartIds;

	// 언락한 스킬 Ids
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSet<FName> UnlockedSkillIds;

	// 언락한 NPC Ids
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSet<FName> UnlockedNPCIds;
};

UCLASS()
class NEOSANCTUM_API UNSPermanentSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// Key : 캐릭터 FName, Value : 세이브 데이터
	UPROPERTY(SaveGame)
	TMap<FName, FNSCharacterSaveData> Characters;

	// 가장 최근에 플레이한 캐릭터 ID
	UPROPERTY(SaveGame)
	FName LastSelectedCharacterId;
};

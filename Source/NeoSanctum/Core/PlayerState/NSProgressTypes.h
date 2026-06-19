// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h" 
#include "NSProgressTypes.generated.h"

class UNSPartDefinition;


USTRUCT(BlueprintType)
struct FNSPartSaveData
{
	GENERATED_BODY()
	// 경로로 직렬화
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSoftObjectPtr<UNSPartDefinition> Definition; 
	UPROPERTY(SaveGame, BlueprintReadOnly)
	ENSPartRarity Rarity = ENSPartRarity::Common;
	// 확장성용(아직 적용 안 함)
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 EnhanceLevel = 0; 
	// 구매 시 1회 롤 → 고정 저장
	UPROPERTY(SaveGame, BlueprintReadOnly)
	float Value = 0.f; 
};

// TMap을 RPC로 보내지 못하기 때문에 배열로 변환용
USTRUCT()
struct FNSNodeLevel
{
	GENERATED_BODY()
	UPROPERTY()
	FName NodeId;
	UPROPERTY() 
	int32 Level = 0;
};

USTRUCT()
struct FNSProgressPayload
{
	GENERATED_BODY()
	// 계정 단위
	UPROPERTY()
	int64 CommonCurrency = 0;
	UPROPERTY()
	TArray<FName> UnlockedNPCIds;
	UPROPERTY()
	TArray<FNSNodeLevel> CommonSkillLevels;
	UPROPERTY() 
	TArray<FNSNodeLevel> PetUpgradeLevels;
	// 활성 캐릭터(직업) 단위
	UPROPERTY()
	FName ActiveCharacterId;
	UPROPERTY()
	int64 JobCurrency = 0;
	UPROPERTY()
	FNSPartSaveData EquippedPart;
	UPROPERTY()
	TArray<FNSNodeLevel> CharacterSkillLevels;
};

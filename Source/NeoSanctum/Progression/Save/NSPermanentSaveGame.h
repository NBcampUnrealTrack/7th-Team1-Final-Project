// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NSPermanentSaveGame.generated.h"

// 캐릭터의 영구 저장 데이터
USTRUCT(BlueprintType)
struct FNSCharacterSaveData
{
	GENERATED_BODY()

	// 직업별 재화
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int64 JobCurrency = 0;   
	
	// 장착 파츠 1개 — 소유 인벤토리의 (정의,등급) 참조, Definition이 null이면 미장착
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSoftObjectPtr<UNSPartDefinition> EquippedPartDefinition;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	ENSPartRarity EquippedPartRarity = ENSPartRarity::Common;
	
	// 캐릭터별 스킬 (스킬종류, 레벨)
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FName,int32> CharacterSkillLevels;
};

UCLASS()
class NEOSANCTUM_API UNSPermanentSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 계정 단위
	// 공용 재화
	UPROPERTY(SaveGame)
	int64 CommonCurrency = 0;
	// NPC 진행도
	UPROPERTY(SaveGame)
	TSet<FName> UnlockedNPCIds;
	// 공용 스킬 정보(스킬종류, 레벨)
	UPROPERTY(SaveGame)
	TMap<FName,int32> CommonSkillLevels;
	// 펫 업그레이드 정보(스킬종류, 레벨)
	UPROPERTY(SaveGame)
	TMap<FName,int32> PetUpgradeLevels;
	// 가장 최근에 플레이한 캐릭터 ID
	UPROPERTY(SaveGame)
	FName LastSelectedCharacterId;
	// 소유 파츠 인벤토리 (계정 공유, 키 = 정의+등급)
	UPROPERTY(SaveGame)
	TArray<FNSPartSaveData> OwnedParts;
	
	// 직업 단위
	// Key : 캐릭터 FName, Value : 세이브 데이터
	UPROPERTY(SaveGame)
	TMap<FName, FNSCharacterSaveData> Characters;
};

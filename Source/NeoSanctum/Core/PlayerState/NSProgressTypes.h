// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSProgressTypes.generated.h"

// TMap을 RPC로 보내지 못하기 때문에 배열로 변환용
USTRUCT()
struct FNSNodeLevel
{
	GENERATED_BODY()
	UPROPERTY() FName NodeId;
	UPROPERTY() int32 Level = 0;
};

USTRUCT()
struct FNSProgressPayload
{
	GENERATED_BODY()
	// 계정 단위
	UPROPERTY() int64 CommonCurrency = 0;
	UPROPERTY() TArray<FName> UnlockedNPCIds;
	UPROPERTY() TArray<FNSNodeLevel> CommonSkillLevels;
	UPROPERTY() TArray<FNSNodeLevel> PetUpgradeLevels;
	// 활성 캐릭터(직업) 단위
	UPROPERTY() FName ActiveCharacterId;
	UPROPERTY() int64 JobCurrency = 0;
	UPROPERTY() TArray<FName> EquippedPartIds;
	UPROPERTY() TArray<FNSNodeLevel> CharacterSkillLevels;
};

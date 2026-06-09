// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "Engine/DataTable.h"
#include "NSCharacterSelectData.generated.h"

class UNSCharacterData;

USTRUCT(BlueprintType)
struct FNSCharacterSelectData : public FTableRowBase
{
	GENERATED_BODY()
	
	//캐릭터 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterSelect")
	FText CharacterName;
	//캐릭터 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterSelect")
	FText CharacterDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterSelect")
	TSoftObjectPtr<UNSCharacterData> CharacterData;
};


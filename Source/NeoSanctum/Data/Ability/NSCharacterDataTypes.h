// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSCharacterDataTypes.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct FNSCharacterAbilityData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Ability",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Ability")
	FGameplayTag InputTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Ability", meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;
};

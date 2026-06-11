// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSCombatStatTypes.generated.h"

USTRUCT(BlueprintType)
struct FNSAbilityBaseStatRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag AbilityTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag StatTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	float BaseValue = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	bool bModifiable = true;
};

UENUM(BlueprintType)
enum class ENSCombatStatModifierOperation : uint8
{
	Add,
	Multiply
};

USTRUCT(BlueprintType)
struct FNSCombatStatModifierRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (AllowedTypes = "NSAugmentData"))
	FPrimaryAssetId SourceDefId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag TargetAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	bool bEnabled = true;
};

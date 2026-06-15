// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSCompanionDefinition.generated.h"

class UGameplayEffect;
class UNSCompanionAbilitySet;

UCLASS(BlueprintType)
class NEOSANCTUM_API UNSCompanionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CompanionTag;
	
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> Icon;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<USkeletalMesh> CompanionMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNSCompanionAbilitySet> AbilitySet;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DefaultStatsEffect;
};

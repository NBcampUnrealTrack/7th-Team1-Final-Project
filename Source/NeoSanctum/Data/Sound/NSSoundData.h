// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoundDataTableRow.h"
#include "Engine/DataAsset.h"
#include "NSSoundData.generated.h"

UCLASS()
class NEOSANCTUM_API UNSSoundData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<UDataTable> SoundDataTable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoundData")
	TArray<FNSSoundCategorySettings> CategorySettings;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NSDataSettings.generated.h"

/**
 * Project Settings > NeoSanctum > Data Settings
 * DataSubsystem이 로드할 데이터 에셋 참조를 설정한다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Data Settings"))
class NEOSANCTUM_API UNSDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("NeoSanctum"); }

	UPROPERTY(EditDefaultsOnly, Config, Category = "NS|Part",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/NeoSanctum.NSPartDefinitionRow"))
	TSoftObjectPtr<UDataTable> PartDefinitionTable;
};

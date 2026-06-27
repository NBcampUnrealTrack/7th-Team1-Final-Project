// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSLevelConfig.generated.h"

class UDataTable;
class UWorld;

/**
 * 인런 레벨 진입에 필요한 맵과 데이터 테이블을 정의하는 Primary Data Asset.
 * 
 * NSGameFlowSubsystem이 TravelMap으로 이동하고,
 * DataSubsystem은 이 설정을 기준으로 레벨별 데이터를 선로딩한다.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSLevelConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	// 이동할 실제 월드. 데이터 선로딩이 끝난 뒤 NSGameFlowSubsystem이 이 맵으로 ServerTravel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level",
		meta = (AssetBundles = "InRunData", AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> TravelMap;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level|Data", meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UDataTable> AbilityBaseStatTable;
	
	// 이번 레벨에서 사용할 증강 후보/Modifier 원본. Definition DA 목록은 이 DT에서 수집.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level|Data", meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UDataTable> AugmentDefinitionTable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level|Data", meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UDataTable> MonsterAttributeTable;
};

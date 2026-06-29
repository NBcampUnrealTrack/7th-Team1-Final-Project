// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSLevelConfig.generated.h"

class UNSAugmentRarityRuleSet;
class UDataTable;
class UWorld;

/**
 * 인런 레벨 진입에 필요한 맵과 데이터 테이블을 정의하는 Primary Data Asset.
 * 
 * 런 전체에서 유지되는 증강/몬스터/난이도 데이터는 UNSRunConfig가 담당하고,
 * 이 클래스는 스테이지가 바뀔 때 교체되는 데이터만 가짐.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSLevelConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	// ServerTravel 대상 맵.
	// 맵 패키지는 DataSubsystem이 선로딩하지 않고 ServerTravel 흐름에 맡김.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level",
		meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> TravelMap;
	
	// 이 스테이지에서 사용할 근접 몬스터 스폰 테이블.
	// ProceduralDungeon 방 로딩과 겹치지 않도록 DataSubsystem의 travel 전 번들 로드 대상에서는 제외.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level|Spawner")
	TSoftObjectPtr<UDataTable> MeleeSpawnerTable;

	// 이 스테이지에서 사용할 원거리 몬스터 스폰 테이블.
	// 실제 사용 시점은 인런 월드 진입 후 스포너 초기화 단계.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Level|Spawner")
	TSoftObjectPtr<UDataTable> RangeSpawnerTable;
};

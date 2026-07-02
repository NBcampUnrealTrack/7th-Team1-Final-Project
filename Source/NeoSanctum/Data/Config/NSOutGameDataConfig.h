// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSOutGameDataConfig.generated.h"

class UDataTable;

/**
 * 거점(OutGame)에서만 필요한 데이터 테이블을 모아두는 Primary Data Asset.
 *
 * NSDataSubsystem이 LoadOutGameData()에서 로드하며,
 * 인런 진입 시 언로드되는 거점 전용 UI/데이터 참조를 관리.
 */
UCLASS()
class NEOSANCTUM_API UNSOutGameDataConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// 거점에서 캐릭터 선택 UI를 구성할 때 사용하는 캐릭터 목록 테이블.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|OutGame|UI",
		meta = (AssetBundles = "OutRunUI"))
	TSoftObjectPtr<UDataTable> CharacterSelectDataTable;
};

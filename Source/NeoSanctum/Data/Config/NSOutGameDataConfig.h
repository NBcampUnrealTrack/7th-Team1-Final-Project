// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSOutGameDataConfig.generated.h"

class UDataTable;

/**
 * OutGame 구간에서만 필요한 데이터 테이블을 모아두는 설정 에셋.
 * 거점 진입 시 NSDataSubsystem이 로드하며, 인런 진입 전까지 유지.
 */
UCLASS()
class NEOSANCTUM_API UNSOutGameDataConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// 거점에서 캐릭터 선택 UI를 구성할 때 사용하는 캐릭터 목록 테이블.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|OutGame|UI",
		meta = (AssetBundles = "OutGameData"))
	TSoftObjectPtr<UDataTable> CharacterSelectDataTable;
};

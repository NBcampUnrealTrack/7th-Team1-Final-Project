// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSCommonDataConfig.generated.h"

class UDataTable;

/**
 * 게임 실행 중 공통으로 유지해야 하는 데이터 테이블과 에셋을 정의하는 Primary Data Asset.
 *
 * NSDataSubsystem이 LoadCommonData()에서 로드하며,
 * 거점과 인런 양쪽에서 참조하는 캐릭터/스킬/성장 데이터를 관리.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSCommonDataConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	// 캐릭터 기본 스탯 또는 Ability 초기 스탯처럼 거점/인런 양쪽에서 필요한 공용 스탯 테이블.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Character", 
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> AbilityBaseStatTable;
	
	// @원종 TODO: 추후 영구 스킬 트리 데이터가 생기면 여기에 추가.
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Progression",
	// 	meta = (AssetBundles = "CommonData"))
	// TSoftObjectPtr<UDataTable> PermanentSkillTreeTable;
};

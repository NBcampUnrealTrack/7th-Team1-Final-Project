// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSCommonDataConfig.generated.h"

class UNSSoundData;
class UDataTable;

/**
 * 게임 실행 중 공통으로 유지해야 하는 데이터 테이블과 에셋을 정의하는 Primary Data Asset.
 *
 * NSDataSubsystem이 LoadCommonData()에서 로드하며,
 * 거점과 인런 양쪽에서 참조하는 모든 데이터를 관리.
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
	
	// 사운드 테이블과 카테고리 볼륨 기본값을 함께 관리하는 공용 사운드 데이터.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Feedback", 
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UNSSoundData> SoundData;
	
	// VFX 모음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Feedback", 
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> VFXDataTable;
	
	// 피격 효과 DT.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Feedback", 
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> HitReactionDataTable;
	
	// 플레이어 공격 피드백 DT.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Feedback", 
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> PlayerAttackFeedbackDataTable;
};

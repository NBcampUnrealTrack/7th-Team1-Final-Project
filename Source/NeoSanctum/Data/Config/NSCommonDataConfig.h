// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSCommonDataConfig.generated.h"

class UGameplayEffect;
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

	// 캐릭터별 초기 Attribute 값. 하나의 Row = 하나의 캐릭터, RowName = CharacterTag.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Character",
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> CharacterBaseStatTable;

	// CharacterBaseStatTable 값을 SetByCaller로 주입하는 공용 초기화 GE.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Character",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<UGameplayEffect> CharacterBaseStatInitEffectClass;

	// GEC에서 사용하는 방어력 감소 배율 공식(y = k / (k + Defense))에 사용하는 상수 k.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Combat")
	float DefenseMitigationConstant = 100.0f;

	// 파츠
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Parts",
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> PartsBaseStatTable;

	// 파츠 슬롯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Parts",
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UDataTable> PartsSlotBaseStatTable;

	// @원종 TODO: 추후 영구 스킬 트리 데이터가 생기면 여기에 추가.
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|Progression",
	// 	meta = (AssetBundles = "CommonData"))
	// TSoftObjectPtr<UDataTable> PermanentSkillTreeTable;

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

	// UIManagerSubsystem이 위젯 RowName으로 위젯 클래스를 찾을 때 사용하는 공용 UI 테이블.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|UI",
		meta = (AssetBundles = "CommonUI"))
	TSoftObjectPtr<UDataTable> UIWidgetDataTable;

	// 캐릭터별 스킬 슬롯 구성을 정의하는 UI 테이블.
	// HUD가 캐릭터 변경 시 스킬 슬롯 RowHandle을 갱신할 때 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|UI",
		meta = (AssetBundles = "CommonUI"))
	TSoftObjectPtr<UDataTable> CharacterSkillUISetTable;

	// 개별 스킬 아이콘/태그 표시 정보를 정의하는 UI 테이블.
	// SkillSlotWidget이 동기 로드 없이 아이콘을 적용할 수 있도록 CommonUI에서 선로드.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|UI",
		meta = (AssetBundles = "CommonUI"))
	TSoftObjectPtr<UDataTable> SkillUIDataTable;

	// 인런/아웃런 재화 아이콘 표시 정보를 정의하는 UI 테이블.
	// GoodsWidget이 생성될 때 동기 로드 없이 아이콘을 적용할 수 있도록 CommonUI에서 선로드.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Common|UI",
		meta = (AssetBundles = "CommonUI"))
	TSoftObjectPtr<UDataTable> GoodsUIDataTable;
};

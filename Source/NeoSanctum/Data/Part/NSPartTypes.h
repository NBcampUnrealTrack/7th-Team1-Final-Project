// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSPartTypes.generated.h"

class UNSPartDefinition;
class USkeletalMesh;

UENUM(BlueprintType)
enum class ENSPartRarity : uint8
{
	Common      UMETA(DisplayName = "커먼"),
	Rare        UMETA(DisplayName = "레어"),
	Epic        UMETA(DisplayName = "에픽"),
	Legendary   UMETA(DisplayName = "레전더리"),
};

USTRUCT(BlueprintType)
struct FNSPartValueRange
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Min = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Max = 0.f;
};

USTRUCT(BlueprintType)
struct FNSPartDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	// DA 참조 -> 세이브/런타임 키와 일치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	TSoftObjectPtr<UNSPartDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part",
		meta = (Categories = "Part.Slot"))
	FGameplayTag PartSlot;

	// 레그 파츠는 false (인런 밸런스상 리롤 불가)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	bool bCanReroll = true;

	// 언락(구매) 영구재화 비용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part",
		meta = (ClampMin = "0"))
	int64 UnlockCost = 0;

	// false면 카탈로그에서 제외
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	bool bEnabled = true;
};

/**
 * 캐릭터 기본 외형에서 이 슬롯에 기본으로 보여줄 메시를 정의하는 항목
 * 시각 전용 메시 하나만 들고 있음.
 */
USTRUCT(BlueprintType)
struct FNSDefaultVisualPartEntry
{
	GENERATED_BODY()

	// 이 항목이 채울 시각 슬롯. 기존 Part.Slot 네임스페이스 그대로 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Part", meta = (Categories = "Part.Slot"))
	FGameplayTag SlotTag;

	// 기본으로 보여줄 메시 - CommonData 번들에 포함시켜 캐릭터 스폰 전에 미리 로드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Part", meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<USkeletalMesh> PartMesh;
};

USTRUCT(BlueprintType)
struct FNSPartSlotRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartSlot",
		meta = (Categories = "Part.Slot"))
	FGameplayTag SlotTag;

	// 슬롯 언락 비용 (CommonCurrency)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartSlot", meta = (ClampMin = "0"))
	int64 UnlockCost = 0;

	// true면 시작부터 언락 (예: Body 슬롯)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartSlot")
	bool bUnlockedByDefault = false;

	// false면 언락 대상에서 제외
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartSlot")
	bool bEnabled = true;
};

UENUM(BlueprintType)
enum class ENSPartUpgradeResult : uint8
{
	RerollDone,
	UpgradeSuccess,
	UpgradeFail,
	NotEnoughCurrency,
	PurchaseDone,
	SoldOut,
};


// 등급별 리롤/등급업 비용, 확률 + 인런 상점 가중치, 가격
USTRUCT(BlueprintType)
struct FNSPartUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	// 이 row가 적용되는 현재 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade")
	ENSPartRarity Rarity = ENSPartRarity::Common;

	// 이 등급의 수치 범위 (장착/리롤/등급업 시 이 범위에서 결정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade")
	FNSPartValueRange ValueRange;

	// 리롤 기본 비용 (임시 재화)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade", meta = (ClampMin = "0"))
	int64 RerollBaseCost = 0;

	// 리롤 1회당 비용 증가치 → 비용 = Base + Increment × RollCount
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade", meta = (ClampMin = "0"))
	int64 RerollCostIncrement = 0;

	// 다음 등급 업그레이드 비용 (임시 재화)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade", meta = (ClampMin = "0"))
	int64 UpgradeCost = 0;

	// 등급업 성공 확률 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UpgradeSuccessChance = 0.5f;

	// 상점 재고 등급 추첨 가중치 (기본 Common 50 / Rare 30 / Epic 15 / Legendary 5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade", meta = (ClampMin = "0.0"))
	float ShopWeight = 0.f;

	// 이 등급 파츠의 상점 구매 가격 (임시 재화)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade", meta = (ClampMin = "0"))
	int64 ShopPrice = 0;
};

/**
 * 파츠 런타임 상태 (레플리케이션/저장 대상)
 * GE 핸들은 UNSPartEquipComponent가 관리
 */
USTRUCT(BlueprintType)
struct FNSPartData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<UNSPartDefinition> DefinitionPtr;

	// 슬롯 식별용
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag Slot;

	UPROPERTY(BlueprintReadWrite)
	ENSPartRarity CurrentRarity = ENSPartRarity::Common;

	UPROPERTY(BlueprintReadWrite)
	float CurrentValue = 0.f;

	UPROPERTY(BlueprintReadWrite)
	int32 RollCount = 0;

	bool IsValid() const { return !DefinitionPtr.IsNull(); }
};

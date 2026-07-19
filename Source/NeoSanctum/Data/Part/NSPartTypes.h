// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NSPartTypes.generated.h"

class UNSPartDefinition;
class USkeletalMesh;
class UMaterialInterface;

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

	// 이 파츠가 대체할 시각 슬롯. 비어있으면 예전처럼 PartSlot 자리에 그대로 표시(과도기 fallback)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part", meta = (Categories = "Part.Visual"))
	FGameplayTag VisualTag;

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
	
	// 이 파츠가 가질 수 있는 스탯 후보
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part", meta = (Categories = "CombatStat"))
	TArray<FGameplayTag> StatTags;

	// StatTag를 공용 파츠 GE로 적용할 때의 연산 방식 (Add = 더하기, Multiply = 배율 곱하기)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;
};

// StatTag 하나당 Row 하나. RowName은 StatTag와 동일해야 함.
// 파츠 상호작용 프롬프트의 스탯 비교 UI가 표시 이름/좋은 방향을 조회하는 용도
USTRUCT(BlueprintType)
struct FNSStatDisplayInfoRow : public FTableRowBase
{
	GENERATED_BODY()

	// FNSPartDefinitionRow::StatTags에 쓰이는 태그와 동일해야 함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Stat", meta = (Categories = "CombatStat"))
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Stat")
	FText DisplayName;

	// true = 값이 오르면 좋음(초록 위쪽 화살표), false = 값이 내리면 좋음(초록 위쪽 화살표는 값 하락 시 표시)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Stat")
	bool bHigherIsBetter = true;

	// 이 스탯의 등급별 수치 범위. 파츠 최종 수치 = 해당 등급 범위에서 직접 롤
	// 키가 없는 등급 = 그 등급에서는 이 스탯이 파츠 후보로 나오지 않음 (의도적 제외, 예: MaxJumpCount는 Legendary만)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Stat")
	TMap<ENSPartRarity, FNSPartValueRange> ValueRangesByRarity;
};

/**
 * 캐릭터 기본 외형에서 이 슬롯에 기본으로 보여줄 메시를 정의하는 항목
 * 시각 전용 메시 하나만 들고 있음.
 */
USTRUCT(BlueprintType)
struct FNSDefaultVisualPartEntry
{
	GENERATED_BODY()

	// 이 항목이 채울 시각 슬롯. 거래/장착용 Part.Slot과 구분되는 시각 전용 네임스페이스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Part", meta = (Categories = "Part.Visual"))
	FGameplayTag PartVisualTag;

	// 기본으로 보여줄 메시 - CommonData 번들에 포함시켜 캐릭터 스폰 전에 미리 로드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Part", meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<USkeletalMesh> PartMesh;

	// 색상 스킨용 머터리얼 오버라이드. 색상 변형이 없는 부위는 비워두면 원본 머터리얼 유지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Part", meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<UMaterialInterface> MaterialOverride;
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

	// UI에서 슬롯 버튼 정렬 순서 (작을수록 위/앞. 예: 바디 0, 암 1, 레그 2)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartSlot")
	int32 SortOrder = 0;
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


// 등급별 리롤/등급업 비용, 확률 + 인런 상점 가중치, 가격 (수치 범위는 FNSStatDisplayInfoRow::ValueRangesByRarity가 소유)
USTRUCT(BlueprintType)
struct FNSPartShopRerollRow : public FTableRowBase
{
	GENERATED_BODY()

	// 이 row가 적용되는 현재 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|PartUpgrade")
	ENSPartRarity Rarity = ENSPartRarity::Common;

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

// 드랍 파츠 액터의 디스폰/바운싱/링VFX 튜닝값. DT에 Row 하나("Default")만 두고 사용
USTRUCT(BlueprintType)
struct FNSDroppedPartConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	// 스폰 후 이 시간이 지나면 자동으로 Destroy
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|DroppedPart", meta = (ClampMin = "0"))
	float DespawnDuration = 15.f;

	// 바운싱 애니메이션 진폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|DroppedPart", meta = (ClampMin = "0"))
	float BobAmplitude = 8.f;

	// 바운싱 애니메이션 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|DroppedPart", meta = (ClampMin = "0"))
	float BobSpeed = 2.f;

	// 파츠를 감싸는 링 VFX ID (DT_VFXDataTable의 Row Name)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|DroppedPart")
	FName RingVFXID;
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
	
	/**
	 * 이 파츠 인스턴스의 확정 스탯
	 * 드롭/상점 생성 시 Row의 StatTags 후보에서 하나가 뽑혀 저장됨
	 */
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag StatTag;

	UPROPERTY(BlueprintReadWrite)
	int32 RollCount = 0;

	bool IsValid() const { return !DefinitionPtr.IsNull(); }
};

// Copyright 2026 One Team. All rights reserved.

#include "NSPartUtils.h"

#include "Engine/AssetManager.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Tag/NSGameplayTags_Part.h"

UNSPartDefinition* NSPartUtils::ResolvePartDefinition(const UObject* WorldContextObject, const TSoftObjectPtr<UNSPartDefinition>& DefinitionPtr)
{
	if (DefinitionPtr.IsNull())
	{
		return nullptr;
	}

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	if (!DataSS)
	{
		// GC 대상(언리처블)으로 표시된 소프트 포인터는 반환하지 않음 — 호출부가 nullptr로 보고 재로드하도록 함
		UNSPartDefinition* Fallback = DefinitionPtr.Get();
		if (!IsValid(Fallback))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PartUtils] DataSS 없음. SoftPtr.Get()=NULL"));
			return nullptr;
		}
		return Fallback;
	}

	const FPrimaryAssetId Id = UAssetManager::Get().GetPrimaryAssetIdForPath(DefinitionPtr.ToSoftObjectPath());
	UNSPartDefinition* Cached = DataSS->GetData<UNSPartDefinition>(Id);

	if (!IsValid(Cached))
	{
		UNSPartDefinition* Fallback = DefinitionPtr.Get();
		if (!IsValid(Fallback))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PartUtils] DataCache 미스 (Id=%s). SoftPtr.Get()=NULL"), *Id.ToString());
			return nullptr;
		}
		return Fallback;
	}

	return Cached;
}

UNSPartDefinition* NSPartUtils::ResolvePartDefinition(const UObject* WorldContextObject, const FNSPartData& Part)
{
	return ResolvePartDefinition(WorldContextObject, Part.DefinitionPtr);
}

const FNSPartDefinitionRow* NSPartUtils::ResolvePartRow(
	const UObject* WorldContextObject,
	const FPrimaryAssetId& DefId)
{
	if (!DefId.IsValid())
	{
		return nullptr;
	}

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	if (!DataSS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartUtils] ResolvePartRow: DataSubsystem 없음 (DefId=%s)"),
			*DefId.ToString());
		return nullptr;
	}

	return DataSS->GetPartRow(DefId);
}

const FNSPartShopRerollRow* NSPartUtils::ResolvePartShopRerollRow(const UObject* WorldContextObject, ENSPartRarity Rarity)
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	if (!DataSS)
	{
		return nullptr;
	}
	return DataSS->GetPartShopRerollRow(Rarity);
}

bool NSPartUtils::GetStatValueRange(
	const UObject* WorldContextObject, const FGameplayTag& StatTag, ENSPartRarity Rarity, FNSPartValueRange& OutRange)
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	const FNSStatDisplayInfoRow* StatInfo = DataSS ? DataSS->FindStatDisplayInfoRow(StatTag) : nullptr;
	if (!StatInfo)
	{
		// 스탯 Row 자체가 없는 건 등급 제외가 아니라 데이터 실수 → 바로 알 수 있게 경고
		UE_LOG(LogTemp, Warning, TEXT("[PartUtils] GetStatValueRange: 스탯 Row 없음 (StatTag=%s)"), *StatTag.ToString());
		return false;
	}

	// 등급 키 없음 = 그 등급에서는 이 스탯이 안 나옴 (의도적 제외라 경고 없음)
	const FNSPartValueRange* Range = StatInfo->ValueRangesByRarity.Find(Rarity);
	if (!Range)
	{
		return false;
	}

	// DT에 Min/Max를 뒤집어 입력해도 롤이 항상 유효하도록 정렬 보장
	OutRange.Min = FMath::Min(Range->Min, Range->Max);
	OutRange.Max = FMath::Max(Range->Min, Range->Max);
	return true;
}

FGameplayTag NSPartUtils::GetPartStatTag(const UObject* WorldContextObject, const FNSPartData& Part)
{
	// 드롭/상점 생성 시점에 확정된 인스턴스 스탯이 단일 소스
	if (Part.StatTag.IsValid())
	{
		return Part.StatTag;
	}

	UNSPartDefinition* Def = ResolvePartDefinition(WorldContextObject, Part);
	const FNSPartDefinitionRow* Row = Def ? ResolvePartRow(WorldContextObject, Def->GetPrimaryAssetId()) : nullptr;
	if (!Row)
	{
		return FGameplayTag();
	}

	const TArray<FGameplayTag> EligibleStatTags = FilterStatTagsByRarity(WorldContextObject, Row->StatTags, Part.CurrentRarity);
	if (EligibleStatTags.Num() == 0)
	{
		return FGameplayTag();
	}
	return EligibleStatTags[0];
}

TArray<FGameplayTag> NSPartUtils::FilterStatTagsByRarity(
	const UObject* WorldContextObject, const TArray<FGameplayTag>& StatTags, ENSPartRarity Rarity)
{
	TArray<FGameplayTag> Eligible;
	Eligible.Reserve(StatTags.Num());

	for (const FGameplayTag& Tag : StatTags)
	{
		// 스탯의 ValueRangesByRarity에 이 등급 키가 있어야 후보로 유효 — 등급 제한이 전부 DT 데이터로 표현됨
		// (예: MaxJumpCount에 Legendary 키만 넣으면 레전더리 전용 스탯이 됨)
		FNSPartValueRange Range;
		if (!GetStatValueRange(WorldContextObject, Tag, Rarity, Range))
		{
			continue;
		}
		Eligible.Add(Tag);
	}

	return Eligible;
}

bool NSPartUtils::ResolveRarityFromTag(const FGameplayTag& RarityTag, ENSPartRarity& OutRarity)
{
	if (RarityTag == NSGameplayTags::Part_Rarity_Common)
	{
		OutRarity = ENSPartRarity::Common;
		return true;
	}
	if (RarityTag == NSGameplayTags::Part_Rarity_Rare)
	{
		OutRarity = ENSPartRarity::Rare;
		return true;
	}
	if (RarityTag == NSGameplayTags::Part_Rarity_Epic)
	{
		OutRarity = ENSPartRarity::Epic;
		return true;
	}
	if (RarityTag == NSGameplayTags::Part_Rarity_Legendary)
	{
		OutRarity = ENSPartRarity::Legendary;
		return true;
	}
	return false;
}

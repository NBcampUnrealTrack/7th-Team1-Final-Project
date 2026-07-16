// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"

class UNSPartDefinition;
struct FNSPartData;
struct FNSPartDefinitionRow;

namespace NSPartUtils
{
	// 파츠 Definition을 NSDataSubsystem 캐시에서 조회, 없으면 .Get() — nullptr 반환 가능
	NEOSANCTUM_API UNSPartDefinition* ResolvePartDefinition(const UObject* WorldContextObject, const TSoftObjectPtr<UNSPartDefinition>& DefinitionPtr);

	// 파츠 Definition을 NSDataSubsystem 캐시에서 조회, 없으면 .Get() — nullptr 반환 가능
	NEOSANCTUM_API UNSPartDefinition* ResolvePartDefinition(const UObject* WorldContextObject, const FNSPartData& Part);

	// DefId로 DataSubsystem 캐시에서 row 조회. 캐시 미스 시 nullptr
	NEOSANCTUM_API const FNSPartDefinitionRow* ResolvePartRow(
		const UObject* WorldContextObject,
		const FPrimaryAssetId& DefId);

	// 등급별 리롤/상점(비용·확률·가중치) row를 NSDataSubsystem 캐시에서 조회. 캐시 미스 시 nullptr
	NEOSANCTUM_API const FNSPartShopRerollRow* ResolvePartShopRerollRow(const UObject* WorldContextObject, ENSPartRarity Rarity);

	// StatTag의 해당 등급 수치 범위를 스탯 표시 DT에서 조회 (Min/Max 정렬 보장)
	// 스탯 Row 자체가 없으면 데이터 실수 → 경고 로그 + false. 등급 키만 없으면 의도적 제외 → 조용히 false
	NEOSANCTUM_API bool GetStatValueRange(
		const UObject* WorldContextObject, const FGameplayTag& StatTag, ENSPartRarity Rarity, FNSPartValueRange& OutRange);

	// 파츠 인스턴스의 확정 스탯 조회, 인스턴스 StatTag 우선
	NEOSANCTUM_API FGameplayTag GetPartStatTag(const UObject* WorldContextObject, const FNSPartData& Part);

	// 등급 조건부 스탯 후보 필터링 (뽑기 전에 자격 없는 후보 제거)
	// 기준은 데이터: 스탯의 ValueRangesByRarity에 해당 등급 키가 있어야 후보로 유효 (키 없음 = 그 등급에서 제외)
	NEOSANCTUM_API TArray<FGameplayTag> FilterStatTagsByRarity(
		const UObject* WorldContextObject, const TArray<FGameplayTag>& StatTags, ENSPartRarity Rarity);

	// Part.Rarity.* 태그를 ENSPartRarity로 변환, 매칭되는 태그가 없으면 false                              
	NEOSANCTUM_API bool ResolveRarityFromTag(const FGameplayTag& RarityTag, ENSPartRarity& OutRarity);
}

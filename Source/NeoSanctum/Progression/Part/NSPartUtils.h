// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
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

	// 등급별 업그레이드/상점 row를 NSDataSubsystem 캐시에서 조회. 캐시 미스 시 nullptr
	NEOSANCTUM_API const FNSPartUpgradeRow* ResolvePartUpgradeRow(const UObject* WorldContextObject, ENSPartRarity Rarity);
}

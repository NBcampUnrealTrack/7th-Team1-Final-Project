// Copyright 2026 One Team. All rights reserved.

#include "NSPartUtils.h"

#include "Engine/AssetManager.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"

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

const FNSPartUpgradeRow* NSPartUtils::ResolvePartUpgradeRow(const UObject* WorldContextObject, ENSPartRarity Rarity)
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	if (!DataSS)
	{
		return nullptr;
	}
	return DataSS->GetPartUpgradeRow(Rarity);
}

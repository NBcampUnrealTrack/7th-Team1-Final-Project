// Copyright 2026 One Team. All rights reserved.

#include "NSPartUtils.h"

#include "Engine/AssetManager.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"

UNSPartDefinition* NSPartUtils::ResolvePartDefinition(const UObject* WorldContextObject, const FNSPartData& Part)
{
	if (Part.DefinitionPtr.IsNull())
	{
		return nullptr;
	}

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject);
	if (!DataSS)
	{
		return Part.DefinitionPtr.Get();
	}

	const FPrimaryAssetId Id = UAssetManager::Get().GetPrimaryAssetIdForPath(Part.DefinitionPtr.ToSoftObjectPath());
	UNSPartDefinition* Cached = DataSS->GetData<UNSPartDefinition>(Id);

	return Cached ? Cached : Part.DefinitionPtr.Get();
}

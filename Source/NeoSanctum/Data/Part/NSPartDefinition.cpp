// Copyright 2026 One Team. All rights reserved.

#include "NSPartDefinition.h"

FPrimaryAssetId UNSPartDefinition::GetPrimaryAssetId() const
{
	// NSDataSubsystem::PartAssetType("NSPartData")과 반드시 일치
	static const FPrimaryAssetType PartDataType = TEXT("NSPartData");
	return FPrimaryAssetId(PartDataType, GetFName());
}

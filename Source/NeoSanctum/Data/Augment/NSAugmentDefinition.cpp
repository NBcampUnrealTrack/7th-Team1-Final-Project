// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentDefinition.h"

FPrimaryAssetId UNSAugmentDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSAugmentData"), GetFName());
}

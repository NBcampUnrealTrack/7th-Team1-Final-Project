// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentPoolDefinition.h"

FPrimaryAssetId UNSAugmentPoolDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSAugmentPool"), GetFName());
}

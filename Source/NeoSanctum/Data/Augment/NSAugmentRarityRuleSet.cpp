// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentRarityRuleSet.h"

FPrimaryAssetId UNSAugmentRarityRuleSet::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSAugmentRarityRuleSet"), GetFName());
}

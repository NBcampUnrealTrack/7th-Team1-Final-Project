// Copyright 2026 One Team. All rights reserved.


#include "NSLevelConfig.h"

FPrimaryAssetId UNSLevelConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSLevelConfig"), GetFName());
}

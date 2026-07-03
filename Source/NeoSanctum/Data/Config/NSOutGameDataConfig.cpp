// Copyright 2026 One Team. All rights reserved.


#include "NSOutGameDataConfig.h"

FPrimaryAssetId UNSOutGameDataConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSOutGameDataConfig"), GetFName());
}

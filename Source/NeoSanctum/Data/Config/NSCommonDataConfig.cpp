// Copyright 2026 One Team. All rights reserved.


#include "NSCommonDataConfig.h"

FPrimaryAssetId UNSCommonDataConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSCommonDataConfig"), GetFName());
}

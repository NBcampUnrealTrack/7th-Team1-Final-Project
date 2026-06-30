// Copyright 2026 One Team. All rights reserved.


#include "NSRunConfig.h"

FPrimaryAssetId UNSRunConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSRunConfig"), GetFName());
}

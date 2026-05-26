// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterData.h"

FPrimaryAssetId UNSCharacterData::GetPrimaryAssetId() const
{
	static const FPrimaryAssetType CharacterDataType = TEXT("CharacterData");
	
	return FPrimaryAssetId(CharacterDataType, GetFName());
}

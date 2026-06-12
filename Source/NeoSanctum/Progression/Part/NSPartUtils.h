// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UNSPartDefinition;
struct FNSPartData;

namespace NSPartUtils
{
	// 파츠 Definition을 NSDataSubsystem 캐시에서 조회, 없으면 .Get() — nullptr 반환 가능
	NEOSANCTUM_API UNSPartDefinition* ResolvePartDefinition(const UObject* WorldContextObject, const FNSPartData& Part);
}

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSSessionType.generated.h"

/**
 *  세션 관련 정보 전달용
 */

// 스팀 친구 1명의 UI 표시용 정보
USTRUCT(BlueprintType)
struct FNSFriendInfo
{
	GENERATED_BODY()

	// 표시 이름
	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	FString DisplayName;

	// 온라인 여부
	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	bool bIsOnline = false;

	// 초대 전송용 NetId 문자열 내부 전용)
	FString NetIdString;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSGuideTextData.generated.h"

/**
 * 아웃런 목표 안내 텍스트 1개
 * UNSOutRunGuideSubsystem::UpdateGuideText 에서 설정 중
 */
USTRUCT(BlueprintType)
struct FNSGuideTextData : public FTableRowBase
{
	GENERATED_BODY()

	// HUD 우측 상단에 표시할 안내 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText GuideText;
};

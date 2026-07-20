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

	// 0보다 크면 이 시간(초)이 지날 때 행동 없이도 자동으로 다음 단계로 넘어감. 0이면 타임아웃 없음(행동/상호작용 전까지 무한 대기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float AutoAdvanceSeconds = 0.0f;
};

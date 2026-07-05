// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSMinimapTypes.generated.h"

//미니맵 캡처 층 설정
USTRUCT(BlueprintType)
struct FNSMinimapCaptureLayerConfig
{
	GENERATED_BODY()

	//층 식별 번호
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	int32 LayerIndex = 0;

	//층 하단 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float FloorZ = 0.0f;

	//층 상단 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float CeilingZ = 3000.0f;
};

//스테이지별 미니맵 층 설정
USTRUCT(BlueprintType)
struct FNSMinimapLayerConfigRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers")
	TArray<FNSMinimapCaptureLayerConfig> CaptureLayers;
};

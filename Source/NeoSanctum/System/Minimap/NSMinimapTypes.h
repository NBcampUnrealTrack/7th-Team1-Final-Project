// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSMinimapTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ENSMinimapIconBoundsPolicy : uint8
{
	Hide,
	ClampToEdge
};

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

//미니맵 아이콘 표시 설정
USTRUCT(BlueprintType)
struct FNSMinimapIconRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon", meta = (ClampMin = "1.0"))
	float Diameter = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon")
	TObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon", meta = (ClampMin = "0.01"))
	float IconScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon")
	int32 DrawPriority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon")
	bool bShowOnAllLayers = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon")
	ENSMinimapIconBoundsPolicy BoundsPolicy = ENSMinimapIconBoundsPolicy::Hide;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon|Clamp", meta = (EditCondition = "BoundsPolicy == ENSMinimapIconBoundsPolicy::ClampToEdge"))
	TObjectPtr<UTexture2D> ClampIconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon|Clamp", meta = (ClampMin = "0.01", EditCondition = "BoundsPolicy == ENSMinimapIconBoundsPolicy::ClampToEdge"))
	float ClampIconScale = 1.0f;

	// ClampIconTexture가 +X 방향을 보고 있지 않을 때 보정하는 각도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icon|Clamp", meta = (EditCondition = "BoundsPolicy == ENSMinimapIconBoundsPolicy::ClampToEdge"))
	float ClampIconRotationOffsetDegrees = 0.0f;
};

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSMinimapConfigDataAsset.generated.h"

class UDataTable;
class UMaterialInterface;

//미니맵 UI 표시 설정
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSMinimapConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	FLinearColor BackgroundColor = FLinearColor(0.01f, 0.01f, 0.01f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	FLinearColor MinimapTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Circle")
	TObjectPtr<UMaterialInterface> CircleMaskMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Circle")
	FName RetainerTextureParameter = TEXT("Texture");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (ClampMin = "500.0"))
	float VisibleWorldWidth = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Orientation")
	bool bRotateMapWithPlayerForward = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Orientation", meta = (EditCondition = "bRotateMapWithPlayerForward"))
	float PlayerForwardUpRotationOffsetDegrees = 0.0f;

	// 캡처 텍스처의 좌우가 UI 좌표와 반대일 때 보정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Orientation")
	bool bMirrorMapHorizontally = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers")
	FLinearColor LowerLayerTint = FLinearColor(0.08f, 0.08f, 0.09f, 0.72f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers")
	FLinearColor UpperLayerTint = FLinearColor(0.62f, 0.66f, 0.70f, 0.48f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Icons")
	TObjectPtr<UDataTable> IconDataTable;
};

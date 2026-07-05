// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSMinimapWidget.generated.h"

UCLASS()
class NEOSANCTUM_API UNSMinimapWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UFUNCTION()
	void HandleMinimapUpdated();

	//미니맵 표시 위치 계산
	FVector2D GetMapDrawPosition(const FVector2D& ViewSize, float MapSize) const;

	//미니맵 층 그리기
	int32 DrawMinimapLayer(
		const struct FNSMinimapLayer& Layer,
		const FVector& PlayerLocation,
		float MapRotationDegrees,
		const FVector2D& MapPosition,
		float MapSize,
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FLinearColor& LayerTint,
		ESlateDrawEffect DrawEffect) const;

	//가장 가까운 아래층 조회
	const struct FNSMinimapLayer* FindNearestLowerLayer(const TArray<struct FNSMinimapLayer>& Layers, int32 CurrentLayerIndex) const;

	//가장 가까운 위층 조회
	const struct FNSMinimapLayer* FindNearestUpperLayer(const TArray<struct FNSMinimapLayer>& Layers, int32 CurrentLayerIndex) const;

	//미니맵 배경 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	FLinearColor BackgroundColor = FLinearColor(0.01f, 0.01f, 0.01f, 0.85f);

	//현재층 표시 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	FLinearColor MinimapTint = FLinearColor::White;

	//화면에 표시할 월드 폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "500.0"))
	float VisibleWorldWidth = 10000.0f;

	//플레이어 전방 기준 지도 회전 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Orientation", meta = (AllowPrivateAccess = "true"))
	bool bRotateMapWithPlayerForward = true;

	//플레이어 전방 기준 회전 보정값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Orientation", meta = (AllowPrivateAccess = "true", EditCondition = "bRotateMapWithPlayerForward"))
	float PlayerForwardUpRotationOffsetDegrees = 0.0f;

	//아래층 표시 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers", meta = (AllowPrivateAccess = "true"))
	FLinearColor LowerLayerTint = FLinearColor(0.08f, 0.08f, 0.09f, 0.72f);

	//위층 표시 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers", meta = (AllowPrivateAccess = "true"))
	FLinearColor UpperLayerTint = FLinearColor(0.62f, 0.66f, 0.70f, 0.48f);
};

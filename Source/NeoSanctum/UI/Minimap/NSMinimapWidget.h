// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/System/Minimap/NSMinimapTypes.h"
#include "NSMinimapWidget.generated.h"

class APawn;
class UNSMinimapIconComponent;
class UNSMinimapConfigDataAsset;
class UNSPlayerProgressComponent;

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

	// 미니맵 기준으로 사용할 로컬 플레이어 Pawn 조회
	const APawn* GetMinimapOwningPawn() const;

	// 미니맵 회전 기준 카메라 Yaw 조회
	float GetMinimapViewYaw() const;

	// 로컬 플레이어 진행도 조회
	const UNSPlayerProgressComponent* GetLocalPlayerProgressComponent() const;

	// 로컬 플레이어 기준 아이콘 표시 여부 판정
	bool ShouldDrawIconForLocalPlayer(const UNSMinimapIconComponent& IconComponent) const;

	// 로컬 플레이어 기준 아이콘 행 결정
	FName ResolveIconRowName(const UNSMinimapIconComponent& IconComponent) const;

	//미니맵 표시 위치 계산
	FVector2D GetMapDrawPosition(const FVector2D& ViewSize, float MapSize) const;

	void ApplyCircleMaskMaterial();

	class URetainerBox* FindOwningRetainerBox() const;

	bool TryResolveIconCenterInCircle(
		const FVector2D& InIconCenter,
		float IconRadius,
		const FVector2D& MapPosition,
		float MapSize,
		ENSMinimapIconBoundsPolicy BoundsPolicy,
		FVector2D& OutIconCenter,
		bool& bOutClamped) const;

	//미니맵 층 그리기
	int32 DrawMinimapLayer(
		const struct FNSMinimapLayer& Layer,
		const FVector& PlayerLocation,
		float MapRotationDegrees,
		const FVector2D& MapPosition,
		float MapSize,
		float InVisibleWorldWidth,
		bool bMirrorHorizontally,
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FLinearColor& LayerTint,
		ESlateDrawEffect DrawEffect) const;

	//미니맵 아이콘 그리기
	int32 DrawMinimapIcons(
		const struct FNSMinimapLayer& CurrentLayer,
		const FVector& PlayerLocation,
		float MapRotationDegrees,
		const FVector2D& MapPosition,
		float MapSize,
		bool bDrawAllLayerIcons,
		bool bDrawOnlyAllLayerIcons,
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	//가장 가까운 아래층 조회
	const struct FNSMinimapLayer* FindNearestLowerLayer(const TArray<struct FNSMinimapLayer>& Layers, int32 CurrentLayerIndex) const;

	//가장 가까운 위층 조회
	const struct FNSMinimapLayer* FindNearestUpperLayer(const TArray<struct FNSMinimapLayer>& Layers, int32 CurrentLayerIndex) const;

	//미니맵 UI 표시 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Config", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSMinimapConfigDataAsset> MinimapConfig;

	bool bCircleMaskMaterialApplied = false;
};

// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/UI/Minimap/NSMinimapWidget.h"

#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Data/Minimap/NSMinimapConfigDataAsset.h"
#include "NeoSanctum/System/Minimap/NSMinimapSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

void UNSMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetClipping(EWidgetClipping::ClipToBounds);

	if (UWorld* World = GetWorld())
	{
		if (UNSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UNSMinimapSubsystem>())
		{
			MinimapSubsystem->OnMinimapUpdated.RemoveDynamic(this, &ThisClass::HandleMinimapUpdated);
			MinimapSubsystem->OnMinimapUpdated.AddDynamic(this, &ThisClass::HandleMinimapUpdated);
		}
	}
}

void UNSMinimapWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (UNSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UNSMinimapSubsystem>())
		{
			MinimapSubsystem->OnMinimapUpdated.RemoveDynamic(this, &ThisClass::HandleMinimapUpdated);
		}
	}

	Super::NativeDestruct();
}

void UNSMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// 이 위젯의 레이아웃이나 렌더 상태가 바뀔 수 있으니 다시 계산/다시 그리도록 하는 함수
	InvalidateLayoutAndVolatility();
}

void UNSMinimapWidget::HandleMinimapUpdated()
{
	InvalidateLayoutAndVolatility();
}

int32 UNSMinimapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const UWorld* World = GetWorld();
	const UNSMinimapSubsystem* MinimapSubsystem = World ? World->GetSubsystem<UNSMinimapSubsystem>() : nullptr;
	if (!MinimapConfig || !MinimapSubsystem || !MinimapSubsystem->HasMinimap())
	{
		return MaxLayerId;
	}

	const APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return MaxLayerId;
	}

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const float MapSize = FMath::Min(ViewSize.X, ViewSize.Y);
	if (MapSize <= 1.0f)
	{
		return MaxLayerId;
	}

	const FVector2D MapPosition = GetMapDrawPosition(ViewSize, MapSize);
	const FVector2D MapDrawSize(MapSize, MapSize);

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	BackgroundBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	BackgroundBrush.OutlineSettings.CornerRadii = FVector4(4.0f, 4.0f, 4.0f, 4.0f);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(MapDrawSize, FSlateLayoutTransform(MapPosition)),
		&BackgroundBrush,
		ESlateDrawEffect::None,
		MinimapConfig->BackgroundColor);

	const FVector PlayerLocation = OwningPawn->GetActorLocation();
	const float MapRotationDegrees = MinimapConfig->bRotateMapWithPlayerForward
		? OwningPawn->GetActorRotation().Yaw - 90.0f + MinimapConfig->PlayerForwardUpRotationOffsetDegrees
		: 0.0f;
	const TArray<FNSMinimapLayer>& Layers = MinimapSubsystem->GetLayers();
	if (!Layers.IsEmpty())
	{
		const int32 CurrentLayerIndex = MinimapSubsystem->GetLayerIndexForWorldZ(PlayerLocation.Z);
		int32 DrawLayerId = LayerId + 1;

		//아래층을 현재층 아래에 먼저 그림
		if (const FNSMinimapLayer* LowerLayer = FindNearestLowerLayer(Layers, CurrentLayerIndex))
		{
			DrawLayerId = DrawMinimapLayer(
				*LowerLayer,
				PlayerLocation,
				MapRotationDegrees,
				MapPosition,
				MapSize,
				MinimapConfig->VisibleWorldWidth,
				MinimapConfig->bMirrorMapHorizontally,
				AllottedGeometry,
				OutDrawElements,
				DrawLayerId,
				MinimapConfig->LowerLayerTint,
				ESlateDrawEffect::None) + 1;
		}

		//현재 플레이어 높이에 맞는 층을 그림
		if (const FNSMinimapLayer* CurrentLayer = MinimapSubsystem->GetLayer(CurrentLayerIndex))
		{
			DrawLayerId = DrawMinimapLayer(
				*CurrentLayer,
				PlayerLocation,
				MapRotationDegrees,
				MapPosition,
				MapSize,
				MinimapConfig->VisibleWorldWidth,
				MinimapConfig->bMirrorMapHorizontally,
				AllottedGeometry,
				OutDrawElements,
				DrawLayerId,
				MinimapConfig->MinimapTint,
				ESlateDrawEffect::None) + 1;
		}

		//위층을 반투명 레이어로 위에 그림
		if (const FNSMinimapLayer* UpperLayer = FindNearestUpperLayer(Layers, CurrentLayerIndex))
		{
			DrawLayerId = DrawMinimapLayer(
				*UpperLayer,
				PlayerLocation,
				MapRotationDegrees,
				MapPosition,
				MapSize,
				MinimapConfig->VisibleWorldWidth,
				MinimapConfig->bMirrorMapHorizontally,
				AllottedGeometry,
				OutDrawElements,
				DrawLayerId,
				MinimapConfig->UpperLayerTint,
				ESlateDrawEffect::None) + 1;
		}

		MaxLayerId = FMath::Max(MaxLayerId, DrawLayerId);
	}
	else if (UTextureRenderTarget2D* RenderTarget = MinimapSubsystem->GetRenderTarget())
	{
		FSlateBrush MinimapBrush;
		MinimapBrush.SetResourceObject(RenderTarget);
		MinimapBrush.ImageSize = MapDrawSize;
		MinimapBrush.DrawAs = ESlateBrushDrawType::Image;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(MapDrawSize, FSlateLayoutTransform(MapPosition)),
			&MinimapBrush,
			ESlateDrawEffect::NoBlending,
			MinimapConfig->MinimapTint);
		MaxLayerId = FMath::Max(MaxLayerId, LayerId + 1);
	}

	return MaxLayerId;
}

FVector2D UNSMinimapWidget::GetMapDrawPosition(const FVector2D& ViewSize, float MapSize) const
{
	return FVector2D(
		(ViewSize.X - MapSize) * 0.5f,
		(ViewSize.Y - MapSize) * 0.5f);
}

const FNSMinimapLayer* UNSMinimapWidget::FindNearestLowerLayer(const TArray<FNSMinimapLayer>& Layers, int32 CurrentLayerIndex) const
{
	//현재층 데이터 조회
	const FNSMinimapLayer* CurrentLayer = nullptr;
	for (const FNSMinimapLayer& Layer : Layers)
	{
		if (Layer.LayerIndex == CurrentLayerIndex)
		{
			CurrentLayer = &Layer;
			break;
		}
	}

	if (!CurrentLayer)
	{
		return nullptr;
	}

	//현재층 바로 아래의 가장 가까운 층 검색
	const FNSMinimapLayer* BestLayer = nullptr;
	float BestCeilingZ = -TNumericLimits<float>::Max();
	for (const FNSMinimapLayer& Layer : Layers)
	{
		if (Layer.LayerIndex == CurrentLayer->LayerIndex)
		{
			continue;
		}

		if (Layer.CeilingZ <= CurrentLayer->FloorZ && Layer.CeilingZ > BestCeilingZ)
		{
			BestCeilingZ = Layer.CeilingZ;
			BestLayer = &Layer;
		}
	}

	return BestLayer;
}

const FNSMinimapLayer* UNSMinimapWidget::FindNearestUpperLayer(const TArray<FNSMinimapLayer>& Layers, int32 CurrentLayerIndex) const
{
	//현재층 데이터 조회
	const FNSMinimapLayer* CurrentLayer = nullptr;
	for (const FNSMinimapLayer& Layer : Layers)
	{
		if (Layer.LayerIndex == CurrentLayerIndex)
		{
			CurrentLayer = &Layer;
			break;
		}
	}

	if (!CurrentLayer)
	{
		return nullptr;
	}

	//현재층 바로 위의 가장 가까운 층 검색
	const FNSMinimapLayer* BestLayer = nullptr;
	float BestFloorZ = TNumericLimits<float>::Max();
	for (const FNSMinimapLayer& Layer : Layers)
	{
		if (Layer.LayerIndex == CurrentLayer->LayerIndex)
		{
			continue;
		}

		if (Layer.FloorZ >= CurrentLayer->CeilingZ && Layer.FloorZ < BestFloorZ)
		{
			BestFloorZ = Layer.FloorZ;
			BestLayer = &Layer;
		}
	}

	return BestLayer;
}

int32 UNSMinimapWidget::DrawMinimapLayer(
	const FNSMinimapLayer& Layer,
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
	ESlateDrawEffect DrawEffect) const
{
	if (!Layer.Texture)
	{
		return LayerId;
	}

	const FVector BoundsSize = Layer.WorldBoundsMax - Layer.WorldBoundsMin;
	const float WorldMapWidth = FMath::Max(BoundsSize.X, BoundsSize.Y);
	const float SafeVisibleWorldWidth = FMath::Max(InVisibleWorldWidth, 500.0f);
	if (WorldMapWidth <= KINDA_SMALL_NUMBER)
	{
		return LayerId;
	}

	const float DrawScale = WorldMapWidth / SafeVisibleWorldWidth;
	const FVector2D LayerDrawSize(MapSize * DrawScale, MapSize * DrawScale);

	//플레이어가 미니맵 중앙에 오도록 층 텍스처 위치 계산
	const float PlayerU = FMath::Clamp((PlayerLocation.X - Layer.WorldBoundsMin.X) / BoundsSize.X, 0.0f, 1.0f);
	const float PlayerV = FMath::Clamp(1.0f - (PlayerLocation.Y - Layer.WorldBoundsMin.Y) / BoundsSize.Y, 0.0f, 1.0f);
	const FVector2D LayerDrawPosition =
		MapPosition +
		FVector2D(MapSize * 0.5f, MapSize * 0.5f) -
		FVector2D(PlayerU * LayerDrawSize.X, PlayerV * LayerDrawSize.Y);

	FSlateBrush LayerBrush;
	LayerBrush.SetResourceObject(Layer.Texture);
	LayerBrush.ImageSize = LayerDrawSize;
	LayerBrush.DrawAs = ESlateBrushDrawType::Image;

	const ESlateDrawEffect EffectiveDrawEffect = Cast<UTextureRenderTarget2D>(Layer.Texture)
		? ESlateDrawEffect::NoBlending
		: DrawEffect;

	FSlateRenderTransform LayerRenderTransform(FQuat2D(FMath::DegreesToRadians(MapRotationDegrees)));
	if (bMirrorHorizontally)
	{
		LayerRenderTransform = Concatenate(
			LayerRenderTransform,
			FSlateRenderTransform(FScale2D(FVector2D(-1.0f, 1.0f))));
	}

	//미니맵 중앙을 기준으로 지도 회전
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(
			LayerDrawSize,
			FSlateLayoutTransform(LayerDrawPosition),
			LayerRenderTransform,
			(MapPosition + FVector2D(MapSize * 0.5f, MapSize * 0.5f) - LayerDrawPosition) / LayerDrawSize),
		&LayerBrush,
		EffectiveDrawEffect,
		LayerTint);

	return LayerId;
}

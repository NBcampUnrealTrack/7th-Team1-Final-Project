// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/UI/Minimap/NSMinimapWidget.h"

#include "Engine/DataTable.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Data/Minimap/NSMinimapConfigDataAsset.h"
#include "NeoSanctum/System/Minimap/NSMinimapIconComponent.h"
#include "NeoSanctum/System/Minimap/NSMinimapSubsystem.h"
#include "NeoSanctum/System/Minimap/NSMinimapTypes.h"
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

	const APawn* OwningPawn = GetMinimapOwningPawn();
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

			DrawLayerId = DrawMinimapIcons(
				*LowerLayer,
				PlayerLocation,
				MapRotationDegrees,
				MapPosition,
				MapSize,
				false,
				AllottedGeometry,
				OutDrawElements,
				DrawLayerId);
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

			DrawLayerId = DrawMinimapIcons(
				*CurrentLayer,
				PlayerLocation,
				MapRotationDegrees,
				MapPosition,
				MapSize,
				true,
				AllottedGeometry,
				OutDrawElements,
				DrawLayerId);
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

			DrawLayerId = DrawMinimapIcons(
				*UpperLayer,
				PlayerLocation,
				MapRotationDegrees,
				MapPosition,
				MapSize,
				false,
				AllottedGeometry,
				OutDrawElements,
				DrawLayerId);
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

const APawn* UNSMinimapWidget::GetMinimapOwningPawn() const
{
	if (const APawn* OwningPawn = GetOwningPlayerPawn())
	{
		return OwningPawn;
	}

	if (const APlayerController* OwningPlayer = GetOwningPlayer())
	{
		return OwningPlayer->GetPawn();
	}

	const UWorld* World = GetWorld();
	const APlayerController* LocalPlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (LocalPlayerController && LocalPlayerController->IsLocalController())
	{
		return LocalPlayerController->GetPawn();
	}

	return nullptr;
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

int32 UNSMinimapWidget::DrawMinimapIcons(
	const FNSMinimapLayer& CurrentLayer,
	const FVector& PlayerLocation,
	float MapRotationDegrees,
	const FVector2D& MapPosition,
	float MapSize,
	bool bDrawAllLayerIcons,
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	if (!MinimapConfig || !MinimapConfig->IconDataTable || MinimapConfig->IconDataTable->GetRowStruct() != FNSMinimapIconRow::StaticStruct())
	{
		return LayerId;
	}

	const UWorld* World = GetWorld();
	const UNSMinimapSubsystem* MinimapSubsystem = World ? World->GetSubsystem<UNSMinimapSubsystem>() : nullptr;
	if (!MinimapSubsystem)
	{
		return LayerId;
	}

	const FVector BoundsSize = CurrentLayer.WorldBoundsMax - CurrentLayer.WorldBoundsMin;
	const float WorldMapWidth = FMath::Max(BoundsSize.X, BoundsSize.Y);
	const float SafeVisibleWorldWidth = FMath::Max(MinimapConfig->VisibleWorldWidth, 500.0f);
	if (WorldMapWidth <= KINDA_SMALL_NUMBER)
	{
		return LayerId;
	}

	const float DrawScale = WorldMapWidth / SafeVisibleWorldWidth;
	const FVector2D LayerDrawSize(MapSize * DrawScale, MapSize * DrawScale);
	const float PlayerU = FMath::Clamp((PlayerLocation.X - CurrentLayer.WorldBoundsMin.X) / BoundsSize.X, 0.0f, 1.0f);
	const float PlayerV = FMath::Clamp(1.0f - (PlayerLocation.Y - CurrentLayer.WorldBoundsMin.Y) / BoundsSize.Y, 0.0f, 1.0f);
	const FVector2D MapCenter = MapPosition + FVector2D(MapSize * 0.5f, MapSize * 0.5f);
	const FVector2D LayerDrawPosition = MapCenter - FVector2D(PlayerU * LayerDrawSize.X, PlayerV * LayerDrawSize.Y);
	FSlateRenderTransform LayerRenderTransform(FQuat2D(FMath::DegreesToRadians(MapRotationDegrees)));
	if (MinimapConfig->bMirrorMapHorizontally)
	{
		LayerRenderTransform = Concatenate(
			LayerRenderTransform,
			FSlateRenderTransform(FScale2D(FVector2D(-1.0f, 1.0f))));
	}

	struct FIconDrawData
	{
		const UNSMinimapIconComponent* Component = nullptr;
		const FNSMinimapIconRow* IconRow = nullptr;
		FVector WorldLocation = FVector::ZeroVector;
	};

	TArray<FIconDrawData> IconsToDraw;
	for (const TWeakObjectPtr<UNSMinimapIconComponent>& IconComponentPtr : MinimapSubsystem->GetIconComponents())
	{
		const UNSMinimapIconComponent* IconComponent = IconComponentPtr.Get();
		if (!IconComponent || !IconComponent->ShouldShowOnMinimap())
		{
			continue;
		}

		const FNSMinimapIconRow* IconRow = MinimapConfig->IconDataTable->FindRow<FNSMinimapIconRow>(
			IconComponent->GetIconRowName(),
			TEXT("MinimapIcon"),
			false);
		if (!IconRow)
		{
			continue;
		}

		const FVector IconWorldLocation = IconComponent->GetIconWorldLocation();
		if (IconRow->bShowOnAllLayers)
		{
			if (!bDrawAllLayerIcons)
			{
				continue;
			}
		}
		else if (!CurrentLayer.ContainsZ(IconWorldLocation.Z))
		{
			continue;
		}

		FIconDrawData DrawData;
		DrawData.Component = IconComponent;
		DrawData.IconRow = IconRow;
		DrawData.WorldLocation = IconWorldLocation;
		IconsToDraw.Add(DrawData);
	}

	IconsToDraw.Sort([](const FIconDrawData& A, const FIconDrawData& B)
	{
		return A.IconRow->DrawPriority < B.IconRow->DrawPriority;
	});

	int32 MaxLayerId = LayerId;
	for (const FIconDrawData& IconData : IconsToDraw)
	{
		const FVector IconWorldLocation = IconData.WorldLocation;
		const float IconU = FMath::Clamp((IconWorldLocation.X - CurrentLayer.WorldBoundsMin.X) / BoundsSize.X, 0.0f, 1.0f);
		const float IconV = FMath::Clamp(1.0f - (IconWorldLocation.Y - CurrentLayer.WorldBoundsMin.Y) / BoundsSize.Y, 0.0f, 1.0f);
		const float IconDiameter = FMath::Max(IconData.IconRow->Diameter, 1.0f);
		const FVector2D IconSize(IconDiameter, IconDiameter);
		const FVector2D IconCenter = LayerDrawPosition + FVector2D(IconU * LayerDrawSize.X, IconV * LayerDrawSize.Y);
		const FVector2D IconPosition = IconCenter - IconSize * 0.5f;

		FSlateBrush IconBrush;
		IconBrush.ImageSize = IconSize;
		IconBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		IconBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		IconBrush.OutlineSettings.CornerRadii = FVector4(
			IconDiameter * 0.5f,
			IconDiameter * 0.5f,
			IconDiameter * 0.5f,
			IconDiameter * 0.5f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MaxLayerId,
			AllottedGeometry.ToPaintGeometry(
				IconSize,
				FSlateLayoutTransform(IconPosition),
				LayerRenderTransform,
				(MapCenter - IconPosition) / IconSize),
			&IconBrush,
			ESlateDrawEffect::None,
			IconData.IconRow->Color);

		++MaxLayerId;
	}

	return MaxLayerId;
}

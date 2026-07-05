// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/System/Minimap/NSMinimapSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"

void UNSMinimapSubsystem::SetMinimap(UTextureRenderTarget2D* NewRenderTarget, const FBox& NewWorldBounds)
{
	//단일 렌더 타겟 미니맵 데이터 갱신
	RenderTarget = NewRenderTarget;
	WorldBoundsMin = NewWorldBounds.Min;
	WorldBoundsMax = NewWorldBounds.Max;
	bHasValidWorldBounds = RenderTarget != nullptr && NewWorldBounds.IsValid != 0;

	OnMinimapUpdated.Broadcast();
}

void UNSMinimapSubsystem::SetMinimapLayers(const TArray<FNSMinimapLayer>& NewLayers)
{
	//다층 미니맵 데이터 갱신
	Layers = NewLayers;

	if (Layers.IsEmpty())
	{
		//빈 레이어 입력 시 미니맵 초기화
		RenderTarget = nullptr;
		WorldBoundsMin = FVector::ZeroVector;
		WorldBoundsMax = FVector::ZeroVector;
		bHasValidWorldBounds = false;
		OnMinimapUpdated.Broadcast();
		return;
	}

	//첫 레이어 기준 대표 월드 범위 설정
	const FNSMinimapLayer& FirstLayer = Layers[0];
	RenderTarget = Cast<UTextureRenderTarget2D>(FirstLayer.Texture);
	WorldBoundsMin = FirstLayer.WorldBoundsMin;
	WorldBoundsMax = FirstLayer.WorldBoundsMax;
	bHasValidWorldBounds = FirstLayer.IsValid();

	OnMinimapUpdated.Broadcast();
}

void UNSMinimapSubsystem::ClearMinimap()
{
	//미니맵 데이터 전체 초기화
	RenderTarget = nullptr;
	WorldBoundsMin = FVector::ZeroVector;
	WorldBoundsMax = FVector::ZeroVector;
	bHasValidWorldBounds = false;
	Layers.Reset();

	OnMinimapUpdated.Broadcast();
}

bool UNSMinimapSubsystem::ProjectWorldToMinimapUV(const FVector& WorldLocation, FVector2D& OutUV) const
{
	if (!HasMinimap())
	{
		return false;
	}

	//전체 미니맵 범위 기준 UV 계산
	const FVector BoundsSize = WorldBoundsMax - WorldBoundsMin;
	if (FMath::IsNearlyZero(BoundsSize.X) || FMath::IsNearlyZero(BoundsSize.Y))
	{
		return false;
	}

	OutUV.X = (WorldLocation.X - WorldBoundsMin.X) / BoundsSize.X;
	OutUV.Y = 1.0f - (WorldLocation.Y - WorldBoundsMin.Y) / BoundsSize.Y;
	OutUV.X = FMath::Clamp(OutUV.X, 0.0f, 1.0f);
	OutUV.Y = FMath::Clamp(OutUV.Y, 0.0f, 1.0f);
	return true;
}

bool UNSMinimapSubsystem::ProjectWorldToMinimapUVForLayer(int32 LayerIndex, const FVector& WorldLocation, FVector2D& OutUV) const
{
	//지정한 층 데이터 조회
	const FNSMinimapLayer* Layer = GetLayer(LayerIndex);
	if (!Layer || !Layer->IsValid())
	{
		return false;
	}

	const FVector BoundsSize = Layer->WorldBoundsMax - Layer->WorldBoundsMin;
	if (FMath::IsNearlyZero(BoundsSize.X) || FMath::IsNearlyZero(BoundsSize.Y))
	{
		return false;
	}

	//층별 월드 범위 기준 UV 계산
	OutUV.X = (WorldLocation.X - Layer->WorldBoundsMin.X) / BoundsSize.X;
	OutUV.Y = 1.0f - (WorldLocation.Y - Layer->WorldBoundsMin.Y) / BoundsSize.Y;
	OutUV.X = FMath::Clamp(OutUV.X, 0.0f, 1.0f);
	OutUV.Y = FMath::Clamp(OutUV.Y, 0.0f, 1.0f);
	return true;
}

const FNSMinimapLayer* UNSMinimapSubsystem::GetLayer(int32 LayerIndex) const
{
	return Layers.FindByPredicate([LayerIndex](const FNSMinimapLayer& Layer)
	{
		return Layer.LayerIndex == LayerIndex;
	});
}

int32 UNSMinimapSubsystem::GetLayerIndexForWorldZ(float WorldZ) const
{
	//높이가 포함되는 층 우선 조회
	for (const FNSMinimapLayer& Layer : Layers)
	{
		if (Layer.ContainsZ(WorldZ))
		{
			return Layer.LayerIndex;
		}
	}

	//포함되는 층이 없으면 가장 가까운 층 조회
	float BestDistance = TNumericLimits<float>::Max();
	int32 BestLayerIndex = INDEX_NONE;
	for (const FNSMinimapLayer& Layer : Layers)
	{
		const float LayerCenterZ = (Layer.FloorZ + Layer.CeilingZ) * 0.5f;
		const float Distance = FMath::Abs(WorldZ - LayerCenterZ);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestLayerIndex = Layer.LayerIndex;
		}
	}

	return BestLayerIndex;
}

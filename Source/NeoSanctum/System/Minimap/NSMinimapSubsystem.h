// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NSMinimapSubsystem.generated.h"

class UTexture;
class UTextureRenderTarget2D;
class UNSMinimapIconComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSMinimapUpdated);

//미니맵 층 데이터
USTRUCT(BlueprintType)
struct FNSMinimapLayer
{
	GENERATED_BODY()

	//층 식별 번호
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Minimap")
	int32 LayerIndex = INDEX_NONE;

	//미니맵에 표시할 층 텍스처
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<UTexture> Texture;

	//층 월드 범위 최소값
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Minimap")
	FVector WorldBoundsMin = FVector::ZeroVector;

	//층 월드 범위 최대값
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Minimap")
	FVector WorldBoundsMax = FVector::ZeroVector;

	//층 하단 높이
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Minimap")
	float FloorZ = 0.0f;

	//층 상단 높이
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Minimap")
	float CeilingZ = 0.0f;

	bool IsValid() const { return Texture != nullptr && LayerIndex != INDEX_NONE; }
	bool ContainsZ(float WorldZ) const { return WorldZ >= FloorZ && WorldZ <= CeilingZ; }
};

UCLASS()
class NEOSANCTUM_API UNSMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//미니맵 데이터 갱신 알림
	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FNSMinimapUpdated OnMinimapUpdated;

	//단일 렌더 타겟 미니맵 설정
	void SetMinimap(UTextureRenderTarget2D* NewRenderTarget, const FBox& NewWorldBounds);

	//다층 텍스처 미니맵 설정
	void SetMinimapLayers(const TArray<FNSMinimapLayer>& NewLayers);

	//미니맵 데이터 초기화
	void ClearMinimap();

	//미니맵 아이콘 컴포넌트 등록
	void RegisterIconComponent(UNSMinimapIconComponent* IconComponent);

	//미니맵 아이콘 컴포넌트 등록 해제
	void UnregisterIconComponent(UNSMinimapIconComponent* IconComponent);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool HasMinimap() const { return bHasValidWorldBounds && (RenderTarget != nullptr || !Layers.IsEmpty()); }

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector GetWorldBoundsMin() const { return WorldBoundsMin; }

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector GetWorldBoundsMax() const { return WorldBoundsMax; }

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool ProjectWorldToMinimapUV(const FVector& WorldLocation, FVector2D& OutUV) const;

	//특정 층 기준 월드 좌표 UV 변환
	bool ProjectWorldToMinimapUVForLayer(int32 LayerIndex, const FVector& WorldLocation, FVector2D& OutUV) const;

	//층 데이터 조회
	const FNSMinimapLayer* GetLayer(int32 LayerIndex) const;

	//전체 층 데이터 조회
	const TArray<FNSMinimapLayer>& GetLayers() const { return Layers; }

	//월드 높이에 맞는 층 번호 조회
	int32 GetLayerIndexForWorldZ(float WorldZ) const;

	//등록된 미니맵 아이콘 컴포넌트 조회
	const TArray<TWeakObjectPtr<UNSMinimapIconComponent>>& GetIconComponents() const { return IconComponents; }

private:
	//단일 캡처용 렌더 타겟
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	//전체 미니맵 월드 범위 최소값
	UPROPERTY(Transient)
	FVector WorldBoundsMin = FVector::ZeroVector;

	//전체 미니맵 월드 범위 최대값
	UPROPERTY(Transient)
	FVector WorldBoundsMax = FVector::ZeroVector;

	//유효한 월드 범위 보유 여부
	UPROPERTY(Transient)
	bool bHasValidWorldBounds = false;

	//다층 미니맵 데이터 보관
	UPROPERTY(Transient)
	TArray<FNSMinimapLayer> Layers;

	//미니맵에 표시할 액터 아이콘 컴포넌트 목록
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UNSMinimapIconComponent>> IconComponents;
};

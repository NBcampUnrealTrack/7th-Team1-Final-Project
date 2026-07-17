// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSCompassBarWidget.generated.h"

class UCommonTextBlock;
class UCanvasPanel;
class UImage;

/**
 * 카메라 방향을 기준으로 눈금과 방위 문자를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSCompassBarWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 목표 방향의 나침반 X 좌표 계산
	UFUNCTION(BlueprintPure, Category = "Compass")
	bool CalculateCompassPosition(
		float ViewYaw,
		float TargetYaw,
		float BarWidth,
		float& OutX) const;

protected:
	// 카메라 Yaw 기준 눈금 및 방위 문자 갱신
	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override;

private:
	// 360도 눈금 스트립 반복 배치
	// 동일한 360도 텍스처를 연속 배치하는 반복 구간 갱신
	void UpdateTickStrips(
		float ViewYaw,
		const FVector2D& CompassSize) const;

	// 8방위 문자 위치 갱신
	void UpdateDirectionLabels(
		float ViewYaw,
		float BarWidth) const;

	// 단일 방위 문자 위치 갱신
	// 가시 각도 범위에 따른 표시 여부 및 화면 좌표 반영
	void UpdateDirectionLabel(
		UCommonTextBlock* DirectionText,
		float RelativeYaw,
		float ViewYaw,
		float BarWidth) const;

	// 단일 눈금 스트립 위치 갱신
	// 반복 구간 연결을 위한 Canvas 슬롯 위치 및 크기 반영
	void UpdateTickStrip(
		UImage* TickStrip,
		float PositionX,
		float StripWidth,
		float FallbackHeight) const;

	// 나침반 좌우 한쪽에 표시할 각도 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "180.0"))
	float VisibleHalfAngle = 90.0f;

	// 월드 북쪽으로 사용할 Yaw 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass", meta = (AllowPrivateAccess = "true"))
	float NorthWorldYaw = 0.0f;

	// 나침반 표시 영역 캔버스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CompassCanvas;

	// 360도 눈금 반복 스트립
	// 기준 360도 스트립
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TickStripA;

	// 오른쪽 첫 번째 반복 스트립
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TickStripB;

	// 오른쪽 두 번째 반복 스트립
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TickStripC;

private:
	// 8방위 문자
	// WBP에서 선택적으로 바인딩하는 방위별 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionN;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionNE;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionE;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionSE;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionS;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionSW;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionW;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionNW;
};

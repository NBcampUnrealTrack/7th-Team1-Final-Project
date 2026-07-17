// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/UI/Compass/NSCompassBarWidget.h"

#include "CommonTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"

bool UNSCompassBarWidget::CalculateCompassPosition(
	const float ViewYaw,
	const float TargetYaw,
	const float BarWidth,
	float& OutX) const
{
	const float SafeHalfAngle = FMath::Clamp(VisibleHalfAngle, 1.0f, 180.0f);
	const float SafeBarWidth = FMath::Max(BarWidth, 0.0f);

	// 현재 카메라와 목표 방위 사이의 최단 회전각 계산
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(ViewYaw, TargetYaw);
	const float HalfWidth = SafeBarWidth * 0.5f;

	// 나침반 중앙 기준 각도 차이의 화면 X 좌표 변환
	OutX = HalfWidth + DeltaYaw / SafeHalfAngle * HalfWidth;

	// 설정된 가시 각도 범위에 따른 목표 방위 표시 여부 판정
	return SafeBarWidth > KINDA_SMALL_NUMBER && FMath::Abs(DeltaYaw) <= SafeHalfAngle;
}

void UNSCompassBarWidget::NativeTick(
	const FGeometry& MyGeometry,
	const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !CompassCanvas)
	{
		return;
	}

	// WBP에 할당된 나침반 표시 영역 크기 조회
	const FVector2D CompassSize = CompassCanvas->GetCachedGeometry().GetLocalSize();
	if (CompassSize.X <= 1.0f || CompassSize.Y <= 1.0f)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	// 로컬 카메라 시점 조회
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 동일한 카메라 Yaw 기준 눈금 및 방위 문자 동기화
	UpdateTickStrips(ViewRotation.Yaw, CompassSize);
	UpdateDirectionLabels(ViewRotation.Yaw, CompassSize.X);
}

void UNSCompassBarWidget::UpdateTickStrips(
	const float ViewYaw,
	const FVector2D& CompassSize) const
{
	// 설정된 가시 각도 범위의 화면 픽셀 비율 계산
	const float SafeHalfAngle = FMath::Clamp(VisibleHalfAngle, 1.0f, 180.0f);
	const float PixelsPerDegree = CompassSize.X / (SafeHalfAngle * 2.0f);
	const float StripWidth = PixelsPerDegree * 360.0f;
	if (StripWidth <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 텍스처 U=0을 북쪽으로 보고 양의 Yaw 방향으로 눈금 진행
	const float HeadingYaw = FRotator::ClampAxis(ViewYaw - NorthWorldYaw);
	const float UnwrappedPositionX = CompassSize.X * 0.5f - HeadingYaw * PixelsPerDegree;

	// 360도 경계 통과 시 연속 배치를 위한 시작 좌표 정규화
	float WrappedPositionX = FMath::Fmod(UnwrappedPositionX, StripWidth);
	if (WrappedPositionX > 0.0f)
	{
		WrappedPositionX -= StripWidth;
	}

	// 현재 화면을 끊김 없이 채우기 위한 동일 스트립 3개 연속 배치
	UpdateTickStrip(TickStripA, WrappedPositionX, StripWidth, CompassSize.Y);
	UpdateTickStrip(TickStripB, WrappedPositionX + StripWidth, StripWidth, CompassSize.Y);
	UpdateTickStrip(TickStripC, WrappedPositionX + StripWidth * 2.0f, StripWidth, CompassSize.Y);
}

void UNSCompassBarWidget::UpdateDirectionLabels(
	const float ViewYaw,
	const float BarWidth) const
{
	// 북쪽 기준 45도 간격으로 8방위 텍스트 위치를 정확하게 맞춤
	UpdateDirectionLabel(DirectionN, 0.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionNE, 45.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionE, 90.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionSE, 135.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionS, 180.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionSW, 225.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionW, 270.0f, ViewYaw, BarWidth);
	UpdateDirectionLabel(DirectionNW, 315.0f, ViewYaw, BarWidth);
}

void UNSCompassBarWidget::UpdateDirectionLabel(
	UCommonTextBlock* DirectionText,
	const float RelativeYaw,
	const float ViewYaw,
	const float BarWidth) const
{
	if (!DirectionText)
	{
		return;
	}

	// 월드 북쪽 보정값을 포함한 방위별 화면 좌표 및 가시성 계산
	float PositionX = 0.0f;
	const bool bVisible = CalculateCompassPosition(
		ViewYaw,
		NorthWorldYaw + RelativeYaw,
		BarWidth,
		PositionX);

	// 표시 범위를 벗어난 방위 텍스트를 보이지 않게 함
	const ESlateVisibility DesiredVisibility = bVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	if (DirectionText->GetVisibility() != DesiredVisibility)
	{
		DirectionText->SetVisibility(DesiredVisibility);
	}

	if (!bVisible)
	{
		return;
	}

	// 방위 문자 중심 기준 Canvas 슬롯 X 좌표 반영
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DirectionText->Slot))
	{
		const FVector2D CurrentPosition = CanvasSlot->GetPosition();
		const FVector2D DesiredPosition(PositionX, CurrentPosition.Y);
		if (!CurrentPosition.Equals(DesiredPosition, 0.01f))
		{
			CanvasSlot->SetPosition(DesiredPosition);
		}
	}
}

void UNSCompassBarWidget::UpdateTickStrip(
	UImage* TickStrip,
	const float PositionX,
	const float StripWidth,
	const float FallbackHeight) const
{
	if (!TickStrip)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TickStrip->Slot);
	if (!CanvasSlot)
	{
		return;
	}

	// 반복 스트립의 명시적 위치 및 크기 제어
	CanvasSlot->SetAutoSize(false);

	const FVector2D CurrentPosition = CanvasSlot->GetPosition();
	const FVector2D DesiredPosition(PositionX, CurrentPosition.Y);
	if (!CurrentPosition.Equals(DesiredPosition, 0.01f))
	{
		CanvasSlot->SetPosition(DesiredPosition);
	}

	// WBP 지정 높이 우선 사용 및 유효하지 않은 경우 표시 영역 높이 적용
	const FVector2D CurrentSize = CanvasSlot->GetSize();
	const float StripHeight = CurrentSize.Y > 1.0f ? CurrentSize.Y : FallbackHeight;
	const FVector2D DesiredSize(StripWidth, StripHeight);
	if (!CurrentSize.Equals(DesiredSize, 0.01f))
	{
		CanvasSlot->SetSize(DesiredSize);
	}
}

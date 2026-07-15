// Copyright 2026 One Team. All rights reserved.


#include "NSMouseCursorWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/SizeBox.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Slate/SceneViewport.h"
#include "UnrealClient.h"

void UNSMouseCursorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯이 다시 생성돼도 델리게이트가 중복 등록되지 않게 먼저 정리.
	FViewport::ViewportResizedEvent.RemoveAll(this);
	FViewport::ViewportResizedEvent.AddUObject(this, &ThisClass::HandleViewPortResized);

	ApplyCursorSize();
}

void UNSMouseCursorWidget::NativeDestruct()
{
	FViewport::ViewportResizedEvent.RemoveAll(this);

	Super::NativeDestruct();
}

void UNSMouseCursorWidget::HandleViewPortResized(FViewport* Viewport, uint32 Unused)
{
	const UWorld* World = GetWorld();
	UGameViewportClient* GameViewportClient = World ? World->GetGameViewport() : nullptr;

	// 에디터의 다른 뷰포트 크기 변경에는 반응하지 않게 게임 뷰포트만 확인.
	if (!GameViewportClient || Viewport != GameViewportClient->GetGameViewport())
	{
		return;
	}

	ApplyCursorSize();
}

void UNSMouseCursorWidget::ApplyCursorSize()
{
	if (!CursorRootSizeBox)
	{
		return;
	}

	// 프로젝트에 이미 설정된 UI DPI Curve를 그대로 사용.
	// 현재 설정은 화면의 짧은 변을 기준으로 크기를 계산.
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

	const FVector2D ScaledImageSize = BaseCursorImageSize * ViewportScale;

	// 소프트웨어 커서는 위젯의 중앙을 마우스 위치로 사용.
	// 이미지가 중앙부터 오른쪽 아래에 배치되도록 루트는 두 배 크기로 잡음.
	CursorRootSizeBox->SetWidthOverride(ScaledImageSize.X * 2.0f);
	CursorRootSizeBox->SetHeightOverride(ScaledImageSize.Y * 2.0f);
}

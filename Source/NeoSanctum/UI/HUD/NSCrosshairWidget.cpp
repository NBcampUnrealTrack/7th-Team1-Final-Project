// Copyright 2026 One Team. All rights reserved.


#include "NSCrosshairWidget.h"
#include "Components/Image.h"

void UNSCrosshairWidget::ShowCrosshair()
{
	//조준점이 필요할때 다시 표시
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSCrosshairWidget::HideCrosshair()
{
	//메뉴, 컷신, 사망 상태등 조준점이 필요없는 상황에서 숨김
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSCrosshairWidget::SetCrosshairColor(FLinearColor Newcolor)
{
	//상태에따라 색상변경
	if (CrosshairImage)
	{
		CrosshairImage->SetColorAndOpacity(Newcolor);
	}
}

void UNSCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//기본 상태에서 조준점 보임
	ShowCrosshair();
}

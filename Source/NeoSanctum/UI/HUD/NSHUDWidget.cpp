// Copyright 2026 One Team. All rights reserved.


#include "NSHUDWidget.h"
#include "NSHPShieldWidget.h"
#include "NSGoodsWidget.h"
#include "NSCrosshairWidget.h"
#include "NSAugmentationWidget.h"

void UNSHUDWidget::UpdateHealthAndShield(
	float CurrentHealth,
	float MaxHealth,
	float CurrentShield,
	float MaxShield
	)
{
	//HP / Shield 위젯이 없으면 갱신X
	if (!HPShieldWidget)
	{
		return;
	}
	
	//HP/Shield UI 갱신
	HPShieldWidget->SetHealth(CurrentHealth, MaxHealth);
	HPShieldWidget->SetShield(CurrentShield, MaxShield);
}

void UNSHUDWidget::UpdateRunInGoods(int32 NewGoodsAmount)
{
	//TODO(영웅): 인런 재화 변경 값
	
	//런 인 재화 갱신
	if (!GoodsWidget)
	{
		return;
	}
	GoodsWidget->SetRunInGoodsAmount(NewGoodsAmount);
}

void UNSHUDWidget::UpdateRunOutGoods(int32 NewGoodsAmount)
{
	
	//런 아웃 재화 갱신
	if (!GoodsWidget)
	{
		return;
	}
	GoodsWidget->SetRunOutGoodsAmount(NewGoodsAmount);
}

void UNSHUDWidget::ResetRunInGoods()
{
	//TODO(영웅): 런 시작 지점 연결
	
	//런 인 재화 초기화
	if (!GoodsWidget)
	{
		return;
	}
	GoodsWidget->ResetRunInGoodsAmount();
}

void UNSHUDWidget::ShowCrosshair()
{
	//조준점이 필요할때 표사
	if (!CrosshairWidget)
	{
		return;
	}
	CrosshairWidget->ShowCrosshair();
}

void UNSHUDWidget::HideCrosshair()
{
	//조준점이 필요없는 상황에 숨긴다
	if (!CrosshairWidget)
	{
		return;
	}
	CrosshairWidget->HideCrosshair();
}

void UNSHUDWidget::SetCrosshairColor(FLinearColor NewColor)
{
	//상황에따른 조준점 색상 변경
	//TODO(영웅): 오버 크리티컬에 따라 색상 변경
	
	if (!CrosshairWidget)
	{
		return;
	}
	CrosshairWidget->SetCrosshairColor(NewColor);
}

void UNSHUDWidget::ShowAugmentation()
{
	//증강 선택 UI를 HUD위에 표시
	if (!AugmentationWidget)
	{
		return;
	}
	AugmentationWidget->ShowAugmentation();
}

void UNSHUDWidget::HideAugmentation()
{
	if (!AugmentationWidget)
	{
		return;
	}
	AugmentationWidget->HideAugmentation();
}


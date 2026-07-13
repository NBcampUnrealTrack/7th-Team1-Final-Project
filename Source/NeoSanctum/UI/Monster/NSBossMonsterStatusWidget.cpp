// Copyright 2026 One Team. All rights reserved.

#include "NSBossMonsterStatusWidget.h"

#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "NSMonsterStatusViewModel.h"

void UNSBossMonsterStatusWidget::BindViewModel(UNSMonsterStatusViewModel* InViewModel)
{
	UnbindViewModel();

	if (!InViewModel)
	{
		return;
	}

	BoundViewModel = InViewModel;
	InViewModel->OnStatusChanged.AddUObject(this, &ThisClass::ApplyStatus);
	ApplyStatus(InViewModel->GetStatus());
}

void UNSBossMonsterStatusWidget::UnbindViewModel()
{
	if (UNSMonsterStatusViewModel* ViewModel = BoundViewModel.Get())
	{
		ViewModel->OnStatusChanged.RemoveAll(this);
	}

	BoundViewModel.Reset();
}

void UNSBossMonsterStatusWidget::ApplyStatus(const FNSMonsterUIStatus& Status)
{
	if (CommonText_BossName)
	{
		CommonText_BossName->SetText(Status.MonsterName);
		CommonText_BossName->SetVisibility(ResolveVisibility(Status.bShowName));
	}

	if (CommonText_HPValue)
	{
		CommonText_HPValue->SetText(Status.HealthText);
		CommonText_HPValue->SetVisibility(ResolveVisibility(Status.bShowHealthText));
	}

	if (CommonText_ShieldValue)
	{
		CommonText_ShieldValue->SetText(Status.ShieldText);
		CommonText_ShieldValue->SetVisibility(ResolveVisibility(Status.bShowShieldText));
	}

	if (CommonText_HitGaugeValue)
	{
		CommonText_HitGaugeValue->SetText(Status.HitGaugeText);
		CommonText_HitGaugeValue->SetVisibility(ResolveVisibility(Status.bShowHitGaugeText));
	}

	if (SizeBox_Shield)
	{
		SizeBox_Shield->SetVisibility(ResolveVisibility(Status.bShowShield));
	}

	if (ProgressBar_HP)
	{
		ProgressBar_HP->SetPercent(Status.HealthPercent);
		ProgressBar_HP->SetVisibility(ResolveVisibility(Status.bShowHealth));
	}

	if (ProgressBar_Shield)
	{
		ProgressBar_Shield->SetPercent(Status.ShieldPercent);
		ProgressBar_Shield->SetVisibility(ResolveVisibility(Status.bShowShield));
	}

	if (ProgressBar_HitGauge)
	{
		ProgressBar_HitGauge->SetPercent(Status.HitGaugePercent);
		ProgressBar_HitGauge->SetVisibility(ResolveVisibility(Status.bShowHitGauge));
	}
}

void UNSBossMonsterStatusWidget::NativeDestruct()
{
	UnbindViewModel();

	Super::NativeDestruct();
}

ESlateVisibility UNSBossMonsterStatusWidget::ResolveVisibility(bool bVisible) const
{
	return bVisible
		       ? ESlateVisibility::HitTestInvisible
		       : ESlateVisibility::Collapsed;
}

// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerWorldStatusWidget.h"

#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "NSPlayerWorldStatusViewModel.h"

// ViewModel을 연결하고 상태 변경 이벤트를 구독하는 함수
void UNSPlayerWorldStatusWidget::BindViewModel(UNSPlayerWorldStatusViewModel* InViewModel)
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

// ViewModel 연결을 해제하는 함수
void UNSPlayerWorldStatusWidget::UnbindViewModel()
{
	if (UNSPlayerWorldStatusViewModel* ViewModel = BoundViewModel.Get())
	{
		ViewModel->OnStatusChanged.RemoveAll(this);
	}

	BoundViewModel.Reset();
}

// ViewModel 상태값을 위젯에 반영하는 함수
void UNSPlayerWorldStatusWidget::ApplyStatus(const FNSPlayerWorldStatusData& StatusData)
{
	const ESlateVisibility NameVisibility = ResolveVisibility(bShowName);
	const ESlateVisibility HealthVisibility = ResolveVisibility(bShowHealth);

	if (SizeBox_Name)
	{
		SizeBox_Name->SetVisibility(NameVisibility);
	}

	if (CommonText_PlayerName)
	{
		CommonText_PlayerName->SetText(StatusData.PlayerName);

		if (!SizeBox_Name)
		{
			CommonText_PlayerName->SetVisibility(NameVisibility);
		}
	}

	if (SizeBox_Health)
	{
		SizeBox_Health->SetVisibility(HealthVisibility);
	}

	if (ProgressBar_HP)
	{
		ProgressBar_HP->SetPercent(StatusData.HealthPercent);

		if (!SizeBox_Health)
		{
			ProgressBar_HP->SetVisibility(HealthVisibility);
		}
	}
}

// 이름과 체력바 표시 여부를 변경하는 함수
void UNSPlayerWorldStatusWidget::SetDisplayOptions(bool bInShowName, bool bInShowHealth)
{
	bShowName = bInShowName;
	bShowHealth = bInShowHealth;

	if (UNSPlayerWorldStatusViewModel* ViewModel = BoundViewModel.Get())
	{
		ApplyStatus(ViewModel->GetStatus());
	}
}

// 위젯 파괴 시 ViewModel 구독을 해제하는 함수
void UNSPlayerWorldStatusWidget::NativeDestruct()
{
	UnbindViewModel();

	Super::NativeDestruct();
}

// bool 값에 따라 표시 상태를 반환하는 함수
ESlateVisibility UNSPlayerWorldStatusWidget::ResolveVisibility(bool bVisible) const
{
	return bVisible
		       ? ESlateVisibility::HitTestInvisible
		       : ESlateVisibility::Collapsed;
}

// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerWorldStatusWidget.h"

#include "Components/ProgressBar.h"
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
	if (ProgressBar_HP)
	{
		ProgressBar_HP->SetPercent(StatusData.HealthPercent);
	}
}

// 위젯 파괴 시 ViewModel 구독을 해제하는 함수
void UNSPlayerWorldStatusWidget::NativeDestruct()
{
	UnbindViewModel();

	Super::NativeDestruct();
}

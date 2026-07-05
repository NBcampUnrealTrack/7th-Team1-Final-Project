// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeWidget.h"

#include "CommonButtonBase.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Debug/Logging/NSLogCategories.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"

void UNSCommonUpgradeWidget::OpenForInteractor(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	OwningController = Interactor;
	AddToViewport();

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked().AddUObject(this, &ThisClass::HandleCloseButtonClicked);
	}

	Interactor->SetShowMouseCursor(true);

	// 게임 입력을 완전히 차단(UIOnly)하고, 전체화면일 때만 마우스를 가둠.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
	Interactor->SetInputMode(InputMode);
}

void UNSCommonUpgradeWidget::CloseWidget()
{
	if (APlayerController* PC = OwningController.Get())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}

FReply UNSCommonUpgradeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleCloseButtonClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UNSCommonUpgradeWidget::HandleCloseButtonClicked()
{
	ANSPlayerController* NSPC = Cast<ANSPlayerController>(OwningController.Get());
	if (!NSPC)
	{
		NS_LOG(LogNS, Warning, "[CommonUpgrade] Close 실패: OwningController가 유효하지 않습니다.");
		return;
	}

	NSPC->CloseInteractionWidget();
}

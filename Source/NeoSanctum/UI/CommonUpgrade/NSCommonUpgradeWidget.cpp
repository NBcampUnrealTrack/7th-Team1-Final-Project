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

	// 다른 상호작용 위젯(Part/Pet)과 달리 화면 전체를 채우는 메뉴이므로 게임 입력을 완전히 차단(UIOnly)한다.
	// TODO(원종): 현재는 테스트의 편의성 때문에 마우스를 가두지 않지만 추후에는 가둠.
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
	// CloseWidget()을 직접 부르지 않고 Controller를 거쳐, ActiveInteractionWidget 포인터 정리까지 Controller가 맡게 한다.
	ANSPlayerController* NSPC = Cast<ANSPlayerController>(OwningController.Get());
	if (!NSPC)
	{
		NS_LOG(LogNS, Warning, "[CommonUpgrade] Close 실패: OwningController가 유효하지 않습니다.");
		return;
	}

	NSPC->CloseInteractionWidget();
}

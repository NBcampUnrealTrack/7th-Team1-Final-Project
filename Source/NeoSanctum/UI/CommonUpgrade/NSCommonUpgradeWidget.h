// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NSCommonUpgradeWidget.generated.h"

class UCommonButtonBase;

/**
 * 공용 업그레이드 콘솔 UI.
 * 현재는 오픈/클로즈 + 입력모드 전환만 담당.
 */
UCLASS()
class NEOSANCTUM_API UNSCommonUpgradeWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;
	virtual void CloseWidget() override;

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 닫기 버튼 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> CloseButton;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	UPROPERTY()
	TWeakObjectPtr<APlayerController> OwningController;
};

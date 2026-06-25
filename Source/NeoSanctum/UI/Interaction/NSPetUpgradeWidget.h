// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NSPetUpgradeWidget.generated.h"

/**
 * 펫 강화 UI (최소 구현)
 * 펫 강화 백엔드 미구현 —> 현재는 오픈/클로즈 + 입력모드 전환만 담당
 */
UCLASS()
class NEOSANCTUM_API UNSPetUpgradeWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Pet")
	virtual void CloseWidget() override;

private:
	TWeakObjectPtr<APlayerController> OwningController;
};

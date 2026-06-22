// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSPetUpgradeWidget.generated.h"

/**
 * 펫 강화 UI (최소 구현)
 * 펫 강화 백엔드 미구현 —> 현재는 오픈/클로즈 + 입력모드 전환만 담당
 */
UCLASS()
class NEOSANCTUM_API UNSPetUpgradeWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 펫 NPC가 호출
	void OpenForInteractor(APlayerController* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Pet")
	void CloseWidget();

private:
	TWeakObjectPtr<APlayerController> OwningController;
};

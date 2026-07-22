// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSInteractableNPCBase.h"
#include "NSCommonUpgradeConsoleNPC.generated.h"

class UNSCommonUpgradeWidget;

/**
 * 공용 업그레이드 콘솔 NPC.
 * 캐릭터 선택 콘솔과 동일하게 해금 요소가 아니므로 항상 상호작용 가능하며,
 * GetInteractionWidgetClass()로 콘솔 UI 위젯 클래스를 반환해 상호작용 시 열리게 함.
 */
UCLASS()
class NEOSANCTUM_API ANSCommonUpgradeConsoleNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual TSubclassOf<UNSNPCInteractionWidgetBase> GetInteractionWidgetClass() const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NPC|UI")
	TSubclassOf<UNSCommonUpgradeWidget> CommonUpgradeWidgetClass;
};

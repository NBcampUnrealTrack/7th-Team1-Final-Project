// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeConsoleNPC.h"
#include "NeoSanctum/UI/Interaction/NSCommonUpgradeWidget.h"

bool ANSCommonUpgradeConsoleNPC::CanInteract_Implementation(APlayerController* Interactor) const
{
	// 공통 업그레이드 콘솔은 캐릭터 선택 콘솔과 동일하게 해금요소가 아니므로 항상 상호작용 가능.
	return Interactor != nullptr;
}

TSubclassOf<UNSNPCInteractionWidgetBase> ANSCommonUpgradeConsoleNPC::GetInteractionWidgetClass() const
{
	return CommonUpgradeWidgetClass;
}

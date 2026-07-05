// Copyright 2026 One Team. All rights reserved.

#include "NSPartUpgradeNPC.h"
#include "NeoSanctum/UI/Interaction/NSPartUpgradeWidget.h"

TSubclassOf<UNSNPCInteractionWidgetBase> ANSPartUpgradeNPC::GetInteractionWidgetClass() const
{
	return PartUpgradeWidgetClass;
}

bool ANSPartUpgradeNPC::CanInteract_Implementation(APlayerController* Interactor) const
{
	return Interactor != nullptr;
}

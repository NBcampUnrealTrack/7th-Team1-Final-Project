// Copyright 2026 One Team. All rights reserved.

#include "NSPetNPC.h"
#include "NeoSanctum/UI/Interaction/NSPetUpgradeWidget.h"

TSubclassOf<UNSNPCInteractionWidgetBase> ANSPetNPC::GetInteractionWidgetClass() const
{
	return PetWidgetClass;
}

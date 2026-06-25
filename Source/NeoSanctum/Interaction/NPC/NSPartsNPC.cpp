// Copyright 2026 One Team. All rights reserved.

#include "NSPartsNPC.h"
#include "NeoSanctum/UI/Interaction/NSPartEquipWidget.h"

TSubclassOf<UNSNPCInteractionWidgetBase> ANSPartsNPC::GetInteractionWidgetClass() const
{
	return PartEquipWidgetClass;
}
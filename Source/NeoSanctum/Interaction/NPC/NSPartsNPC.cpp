// Copyright 2026 One Team. All rights reserved.

#include "NSPartsNPC.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/UI/Interaction/NSPartEquipWidget.h"

bool ANSPartsNPC::OnInteract_Implementation(APlayerController* Interactor)
{
	if (!Interactor || !PartEquipWidgetClass)
	{
		return false;
	}

	UNSPartEquipWidget* Widget = CreateWidget<UNSPartEquipWidget>(Interactor, PartEquipWidgetClass);
	if (!Widget)
	{
		return false;
	}

	Widget->OpenForInteractor(Interactor);
	return true;
}
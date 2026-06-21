// Copyright 2026 One Team. All rights reserved.

#include "NSPetNPC.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/UI/Interaction/NSPetUpgradeWidget.h"

bool ANSPetNPC::OnInteract_Implementation(APlayerController* Interactor)
{
	if (!Interactor || !PetWidgetClass)
	{
		return false;
	}

	UNSPetUpgradeWidget* Widget = CreateWidget<UNSPetUpgradeWidget>(Interactor, PetWidgetClass);
	if (!Widget)
	{
		return false;
	}

	Widget->OpenForInteractor(Interactor);
	return true;
}

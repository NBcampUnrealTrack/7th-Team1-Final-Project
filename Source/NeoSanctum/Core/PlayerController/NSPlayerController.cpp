// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSOutGameModeInterface.h"

ANSPlayerController::ANSPlayerController()
{
	
}

void ANSPlayerController::Server_RequestStartRun_Implementation()
{
	if (HasAuthority())
	{
		AGameModeBase* CurrentGameMode = GetWorld()->GetAuthGameMode();
		
		if (CurrentGameMode && CurrentGameMode->Implements<UNSOutGameInterface>())
		{
			INSOutGameInterface::Execute_RequestStartRun(CurrentGameMode);
		}
	}
}

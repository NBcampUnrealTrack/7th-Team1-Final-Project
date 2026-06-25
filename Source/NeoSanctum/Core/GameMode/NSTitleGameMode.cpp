// Copyright 2026 One Team. All rights reserved.


#include "NSTitleGameMode.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"

ANSTitleGameMode::ANSTitleGameMode()
{
	bUseSeamlessTravel = false;
	
	PlayerControllerClass = ANSPlayerController::StaticClass();
}

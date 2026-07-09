// Copyright 2026 One Team. All rights reserved.


#include "NSIntroGameMode.h"
#include "NeoSanctum/Core/PlayerController/NSIntroPlayerController.h"


ANSIntroGameMode::ANSIntroGameMode()
{
	PlayerControllerClass = ANSIntroPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	bUseSeamlessTravel = false;
}
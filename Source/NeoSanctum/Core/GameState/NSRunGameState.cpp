// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameState.h"
#include "NeoSanctum/Core/Component/NSRunGameDataComponent.h"

ANSRunGameState::ANSRunGameState()
{
	RunGameDataComponent = CreateDefaultSubobject<UNSRunGameDataComponent>(TEXT("RunGameDataComponent"));
}

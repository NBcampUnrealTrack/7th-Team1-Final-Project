// Copyright 2026 One Team. All rights reserved.

#include "NSBossMonsterPresenter.h"

void UNSBossMonsterPresenter::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;
}

void UNSBossMonsterPresenter::Shutdown()
{
	OwningLocalPlayer.Reset();
}
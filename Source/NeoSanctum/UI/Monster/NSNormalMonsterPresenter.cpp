// Copyright 2026 One Team. All rights reserved.

#include "NSNormalMonsterPresenter.h"

void UNSNormalMonsterPresenter::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;
}

void UNSNormalMonsterPresenter::Shutdown()
{
	OwningLocalPlayer.Reset();
}

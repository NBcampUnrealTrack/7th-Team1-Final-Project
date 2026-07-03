// Copyright 2026 One Team. All rights reserved.

#include "NSBossPawnBase.h"

#include "NeoSanctum/Combat/Component/NSBossModeComponent.h"
#include "NeoSanctum/Combat/Component/NSBossTargetComponent.h"

ANSBossPawnBase::ANSBossPawnBase()
{
	BossModeComponent = CreateDefaultSubobject<UNSBossModeComponent>(TEXT("BossModeComponent"));
	BossTargetComponent = CreateDefaultSubobject<UNSBossTargetComponent>(TEXT("BossTargetComponent"));
}

void ANSBossPawnBase::BeginPlay()
{
	Super::BeginPlay();

	if (BossModeComponent)
	{
		BossModeComponent->InitializeMode();
	}
}

void ANSBossPawnBase::ApplyDeadState()
{
	Super::ApplyDeadState();

	if (BossTargetComponent)
	{
		BossTargetComponent->ResetTargets();
	}
}

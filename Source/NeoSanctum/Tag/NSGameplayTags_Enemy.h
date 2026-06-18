// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Enemy State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_Combat);

	// Enemy Action
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_BasicMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_DoubleMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TripleMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_StrongMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RangerAttack);

	// Enemy Event
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Enemy_Hit);
}

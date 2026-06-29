// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_State.h"

namespace NSGameplayTags
{
	// State
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Invincible, "State.Invincible");
	UE_DEFINE_GAMEPLAY_TAG(State_Dashing, "State.Dashing");
	UE_DEFINE_GAMEPLAY_TAG(State_Reloading, "State.Reloading");
	UE_DEFINE_GAMEPLAY_TAG(State_Deactivate_HandIK, "State.Deactivate.HandIK");
	
	// Shield Regen
	UE_DEFINE_GAMEPLAY_TAG(State_Shield_Broken, "State.Shield.Broken");
	UE_DEFINE_GAMEPLAY_TAG(State_Shield_Recharging, "State.Shield.Recharging");
	UE_DEFINE_GAMEPLAY_TAG(State_Damage_RecentlyTaken, "State.Damage.RecentlyTaken");
	
	// Barrier
	UE_DEFINE_GAMEPLAY_TAG(State_Barrier_Activated, "State.Barrier.Activated");
	
	// Buff State
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_RangerSpeedBuff, "State.Buff.RangerSpeedBuff");
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_EngineerSpeedBuff, "State.Buff.EngineerSpeedBuff");
}

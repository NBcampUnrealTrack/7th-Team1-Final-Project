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
	
	// Buff State
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_RangerSpeedBuff, "State.Buff.RangerSpeedBuff");
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_EngineerSpeedBuff, "State.Buff.EngineerSpeedBuff");
}

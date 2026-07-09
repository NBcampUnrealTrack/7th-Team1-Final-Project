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
	UE_DEFINE_GAMEPLAY_TAG(State_DashAttackWindow, "State.DashAttackWindow");
	UE_DEFINE_GAMEPLAY_TAG(State_Input_BlockInputMove, "State.Input.BlockInputMove");
	UE_DEFINE_GAMEPLAY_TAG(State_Vanguard_Attacking, "State.Vanguard.Attacking");
	UE_DEFINE_GAMEPLAY_TAG(State_Vanguard_Guarding, "State.Vanguard.Guarding");
	UE_DEFINE_GAMEPLAY_TAG(State_Vanguard_BarrierAttackWindow, "State.Vanguard.BarrierAttackWindow");
	UE_DEFINE_GAMEPLAY_TAG(State_Vanguard_ChargingDashAttack, "State.Vanguard.ChargingDashAttack");
	UE_DEFINE_GAMEPLAY_TAG(State_Vanguard_ComboInputWindow, "State.Vanguard.ComboInputWindow");
	
	// Shield Regen
	UE_DEFINE_GAMEPLAY_TAG(State_Shield_Broken, "State.Shield.Broken");
	UE_DEFINE_GAMEPLAY_TAG(State_Shield_RechargeCooldown, "State.Shield.RechargeCooldown");
	UE_DEFINE_GAMEPLAY_TAG(State_Shield_Recharging, "State.Shield.Recharging");
	
	// Barrier
	UE_DEFINE_GAMEPLAY_TAG(State_Barrier_Activated, "State.Barrier.Activated");
	
	// Buff State
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_RangerSpeedBuff, "State.Buff.RangerSpeedBuff");
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_EngineerSpeedBuff, "State.Buff.EngineerSpeedBuff");
	UE_DEFINE_GAMEPLAY_TAG(State_Buff_VanguardBuff, "State.Buff.VanguardBuff");
}

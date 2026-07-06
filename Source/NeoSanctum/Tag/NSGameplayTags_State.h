// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dashing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Reloading);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Deactivate_HandIK);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_DashAttackWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_Attacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_Guarding);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_BarrierAttackWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_ChargingDashAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_ComboInputWindow);
	
	// Shield Regen
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shield_Broken);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shield_RechargeCooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shield_Recharging);
	
	// Barrier
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Barrier_Activated);
	
	// Buff State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_RangerSpeedBuff);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_EngineerSpeedBuff);
}

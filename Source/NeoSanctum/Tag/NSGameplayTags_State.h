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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ThrowProjectile_Active);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ThrowProjectile_Releasing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Input_BlockInputMove);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_Attacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_Flickering);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_AirSlamming);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_BarrierAttackWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_ChargingDashAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_ComboInputWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Vanguard_ThrowBarrierField);
	
	// Shield Regen
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shield_Broken);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shield_RechargeCooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shield_Recharging);
	
	// Barrier
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Barrier_Activated);
	
	// Buff State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_RangerSpeedBuff);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_EngineerSpeedBuff);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_VanguardBuff);
}

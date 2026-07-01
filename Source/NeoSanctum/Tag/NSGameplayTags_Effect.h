// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Turret SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_FireRate_Engineer_SpawnTurret);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_AttackRange_Engineer_SpawnTurret);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_DetectionRange_Engineer_SpawnTurret);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Accuracy_Engineer_SpawnTurret);
	
	// Attribute SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_MaxHealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_MaxShield);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Defense);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_MaxAmmo);
	
	// Buff SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Buff_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Buff_ReloadSpeed);
	
	// SkillSlot Recharge SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Recharge_Skill1Count);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Recharge_Skill2Count);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Recharge_Skill3Count);

	// Cooldown
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cooldown_Ranger_ProjectileShot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cooldown_Ranger_Grenade);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cooldown_BuffBase);

	// Damage SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Base);
}

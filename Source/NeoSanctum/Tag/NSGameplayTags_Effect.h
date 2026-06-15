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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Health);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_MaxHealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Defense);
	// Buff SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Buff_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Buff_ReloadSpeed);

	// Cooldown
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cooldown_Ranger_ProjectileShot);

	// Damage SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Base);
}

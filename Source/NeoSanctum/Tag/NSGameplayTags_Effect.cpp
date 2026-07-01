// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Effect.h"

namespace NSGameplayTags
{
	// Turret SetByCaller
	UE_DEFINE_GAMEPLAY_TAG(Effect_FireRate_Engineer_SpawnTurret, "Effect.FireRate.Engineer.SpawnTurret");
	UE_DEFINE_GAMEPLAY_TAG(Effect_AttackRange_Engineer_SpawnTurret, "Effect.AttackRange.Engineer.SpawnTurret");
	UE_DEFINE_GAMEPLAY_TAG(Effect_DetectionRange_Engineer_SpawnTurret, "Effect.DetectionRange.Engineer.SpawnTurret");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Accuracy_Engineer_SpawnTurret, "Effect.Accuracy.Engineer.SpawnTurret");
	
	// Attribute SetByCaller
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxHealth, "Effect.SetByCaller.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxShield, "Effect.SetByCaller.MaxShield");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_Defense, "Effect.SetByCaller.Defense");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxAmmo, "Effect.SetByCaller.MaxAmmo");
	
	// Buff SetByCaller
	UE_DEFINE_GAMEPLAY_TAG(Effect_Buff_FireRate, "Effect.Buff.FireRate");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Buff_ReloadSpeed, "Effect.Buff.ReloadSpeed");
	
	// SkillSlot Recharge SetByCaller
	UE_DEFINE_GAMEPLAY_TAG(Effect_Recharge_Skill1Count, "Effect.Recharge.Skill1Count");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Recharge_Skill2Count, "Effect.Recharge.Skill2Count");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Recharge_Skill3Count, "Effect.Recharge.Skill3Count");
	
	// Cooldown
	UE_DEFINE_GAMEPLAY_TAG(Effect_Cooldown_Ranger_ProjectileShot, "Effect.Cooldown.Ranger.ProjectileShot");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Cooldown_Ranger_Grenade, "Effect.Cooldown.Ranger.Grenade");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Cooldown_BuffBase, "Effect.Cooldown.BuffBase");

	// Damage SetByCaller
	UE_DEFINE_GAMEPLAY_TAG(Effect_Damage_Base, "Effect.Damage.Base");
}

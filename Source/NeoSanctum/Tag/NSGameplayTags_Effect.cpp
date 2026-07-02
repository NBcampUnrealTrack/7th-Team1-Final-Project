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
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_BaseDamage_Add, "Effect.SetByCaller.BaseDamage.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_BaseDamage_Multiply, "Effect.SetByCaller.BaseDamage.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxHealth_Add, "Effect.SetByCaller.MaxHealth.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxHealth_Multiply, "Effect.SetByCaller.MaxHealth.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxShield_Add, "Effect.SetByCaller.MaxShield.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxShield_Multiply, "Effect.SetByCaller.MaxShield.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_Defense_Add, "Effect.SetByCaller.Defense.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_Defense_Multiply, "Effect.SetByCaller.Defense.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_ShieldRechargeRate_Add, "Effect.SetByCaller.ShieldRechargeRate.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_ShieldRechargeRate_Multiply, "Effect.SetByCaller.ShieldRechargeRate.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_ShieldRechargeCooldown_Add, "Effect.SetByCaller.ShieldRechargeCooldown.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_ShieldRechargeCooldown_Multiply, "Effect.SetByCaller.ShieldRechargeCooldown.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxDashCount_Add, "Effect.SetByCaller.MaxDashCount.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_DashRegenRate_Add, "Effect.SetByCaller.DashRegenRate.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_DashRegenRate_Multiply, "Effect.SetByCaller.DashRegenRate.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxAmmo_Add, "Effect.SetByCaller.MaxAmmo.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxAmmo_Multiply, "Effect.SetByCaller.MaxAmmo.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxSkill1Count_Add, "Effect.SetByCaller.MaxSkill1Count.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxSkill2Count_Add, "Effect.SetByCaller.MaxSkill2Count.Add");
	UE_DEFINE_GAMEPLAY_TAG(Effect_SetByCaller_MaxSkill3Count_Add, "Effect.SetByCaller.MaxSkill3Count.Add");
	
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

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
	UE_DEFINE_GAMEPLAY_TAG(Effect_Health, "Effect.Health");
	UE_DEFINE_GAMEPLAY_TAG(Effect_MaxHealth, "Effect.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Defense, "Effect.Defense");

	// Cooldown
	UE_DEFINE_GAMEPLAY_TAG(Effect_Cooldown_Ranger_ProjectileShot, "Effect.Cooldown.Ranger.ProjectileShot");

	// Damage SetByCaller
	UE_DEFINE_GAMEPLAY_TAG(Effect_Damage_Base, "Effect.Damage.Base");
}

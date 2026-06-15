// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// FireRate 발사속도
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_FireRate_Engineer_SpawnTurret);

	// AttackRange 사정거리
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_AttackRange_Engineer_SpawnTurret);

	// DetectionRange 탐지범위
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_DetectionRange_Engineer_SpawnTurret);

	// Accuracy 정확도
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Accuracy_Engineer_SpawnTurret);

	// Cooldown
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cooldown_Ranger_ProjectileShot);
	
	// Damage SetByCaller
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Base);
}

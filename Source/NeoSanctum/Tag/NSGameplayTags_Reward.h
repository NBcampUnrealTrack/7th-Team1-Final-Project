// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Types
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Type_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Type_Augment);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Type_Currency);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Type_Part);
	
	// Rewards
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Trigger_NormalKill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Trigger_EliteKill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Trigger_BossKill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Trigger_Chest);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward_Trigger_LevelUp);
}

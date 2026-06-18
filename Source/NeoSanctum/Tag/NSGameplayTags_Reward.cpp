// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Reward.h"

namespace NSGameplayTags
{
	// Types
	UE_DEFINE_GAMEPLAY_TAG(Reward_Type_None, "Reward.Type.None");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Type_Augment, "Reward.Type.Augment");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Type_Currency, "Reward.Type.Currency");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Type_Part, "Reward.Type.Part");
	
	// Rewards
	UE_DEFINE_GAMEPLAY_TAG(Reward_Trigger_NormalKill, "Reward.Trigger.NormalKill");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Trigger_EliteKill, "Reward.Trigger.EliteKill");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Trigger_BossKill, "Reward.Trigger.BossKill");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Trigger_Chest, "Reward.Trigger.Chest");
	UE_DEFINE_GAMEPLAY_TAG(Reward_Trigger_LevelUp, "Reward.Trigger.LevelUp");
}

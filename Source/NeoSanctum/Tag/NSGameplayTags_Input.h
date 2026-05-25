// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Native Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Look);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Sprint);
	
	// Ability Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_BaseAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_ActiveSkill1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_ActiveSkill2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_ActiveSkill3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Dash);
}

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Native Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Look);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Sprint);
	
	// Ability Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_BaseAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_ActiveSkill1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_ActiveSkill2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_ActiveSkill3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);
}

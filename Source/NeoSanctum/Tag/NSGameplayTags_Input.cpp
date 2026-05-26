// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Input.h"

namespace NSGameplayTags
{
	// Native Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Move, "Input.Native.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Look, "Input.Native.Look");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Jump, "Input.Native.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Sprint, "Input.Native.Sprint");
	
	// Ability Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_BaseAttack, "Input.Ability.BaseAttack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_ActiveSkill1, "Input.Ability.ActiveSkill1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_ActiveSkill2, "Input.Ability.ActiveSkill2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_ActiveSkill3, "Input.Ability.ActiveSkill3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Dash, "Input.Ability.Dash");
}

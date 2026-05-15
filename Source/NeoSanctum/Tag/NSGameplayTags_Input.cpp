// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Input.h"

namespace NSGameplayTags
{
	// Native Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Move, "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Look, "Input.Look");
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Sprint, "Input.Sprint");
	
	// Ability Input
	UE_DEFINE_GAMEPLAY_TAG(Input_BaseAttack, "Input.BaseAttack");
	UE_DEFINE_GAMEPLAY_TAG(Input_ActiveSkill1, "Input.ActiveSkill1");
	UE_DEFINE_GAMEPLAY_TAG(Input_ActiveSkill2, "Input.ActiveSkill2");
	UE_DEFINE_GAMEPLAY_TAG(Input_ActiveSkill3, "Input.ActiveSkill3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Dodge, "Input.Dodge");
}

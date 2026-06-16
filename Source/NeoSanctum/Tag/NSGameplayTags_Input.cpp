// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Input.h"

namespace NSGameplayTags
{
	// Input Mode
	UE_DEFINE_GAMEPLAY_TAG(InputMode_Gameplay, "InputMode.Gameplay");
	UE_DEFINE_GAMEPLAY_TAG(InputMode_UI, "InputMode.UI");
	UE_DEFINE_GAMEPLAY_TAG(InputMode_DeathSpectator, "InputMode.DeathSpectator");
	UE_DEFINE_GAMEPLAY_TAG(InputMode_Augment, "InputMode.Augment");

	// Native Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Move, "Input.Native.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Look, "Input.Native.Look");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Jump, "Input.Native.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Sprint, "Input.Native.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Interact, "Input.Native.Interact");
	
	
	// Native Death Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Death_PrevPlayer, "Input.Native.Death.PrevPlayer");
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Death_NextPlayer, "Input.Native.Death.NextPlayer");
	
	// Ability Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_BaseAttack, "Input.Ability.BaseAttack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_ActiveSkill1, "Input.Ability.ActiveSkill1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_ActiveSkill2, "Input.Ability.ActiveSkill2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_ActiveSkill3, "Input.Ability.ActiveSkill3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Dash, "Input.Ability.Dash");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Reload, "Input.Ability.Reload");

	// Augment Input
	UE_DEFINE_GAMEPLAY_TAG(Input_Augment_Card1, "Input.Augment.Card1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Augment_Card2, "Input.Augment.Card2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Augment_Card3, "Input.Augment.Card3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Augment_Reroll, "Input.Augment.Reroll");
	UE_DEFINE_GAMEPLAY_TAG(Input_Augment_TogglePanel, "Input.Augment.TogglePanel");
}

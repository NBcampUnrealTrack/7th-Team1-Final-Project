// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Input Mode
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_Gameplay);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_UI);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_DeathSpectator);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_Augment);

	// Native Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Look);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Sprint);
	
	//Native Death Inpu
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Death_PrevPlayer);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Death_NextPlayer);
	
	// Ability Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_BaseAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_ActiveSkill1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_ActiveSkill2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_ActiveSkill3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Dash);
	
	// Augment Input
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Augment_Card1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Augment_Card2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Augment_Card3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Augment_Reroll);

}

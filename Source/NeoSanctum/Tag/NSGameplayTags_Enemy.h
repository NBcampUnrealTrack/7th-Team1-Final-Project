// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Enemy State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_Combat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_HitReacting);
	
	// Enemy TitanWalker State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_TitanWalker_Mobile);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_TitanWalker_Siege);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_TitanWalker_ExposedCore);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_TitanWalker_DestroyedLeg);

	// Enemy MotherShip State
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_MotherShip_Phase1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_MotherShip_Phase2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Enemy_MotherShip_Charge);
	
	// Enemy MotherShip Action
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_MotherShip_SpawnDrone);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_MotherShip_BombingRun);
	
	// Enemy Action
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_BasicMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_DoubleMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TripleMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_StrongMelee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RangerAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_FlyingBurstAttack);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TitanWalker_MachineGun);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TitanWalker_Flame);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TitanWalker_Bombard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_TitanWalker_Laser);
	
	// Enemy Action - Common
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_HitReaction);

	// Enemy Event
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Enemy_Hit);
}

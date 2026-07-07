#include "NSGameplayTags_Enemy.h"

namespace NSGameplayTags
{
	// Enemy State
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_Combat, "State.Enemy.Combat");
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_HitReacting, "State.Enemy.HitReacting");
	
	// Enemy TitanWalker State
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_TitanWalker_Mobile, "State.Enemy.TitanWalker.Mobile");
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_TitanWalker_Siege, "State.Enemy.TitanWalker.Siege");
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_TitanWalker_ExposedCore, "State.Enemy.TitanWalker.ExposedCore");
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_TitanWalker_DestroyedLeg, "State.Enemy.TitanWalker.DestroyedLeg");

	// Enemy MotherShip State
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_MotherShip_Phase1, "State.Enemy.MotherShip.Phase1");
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_MotherShip_Phase2, "State.Enemy.MotherShip.Phase2");
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_MotherShip_Charge, "State.Enemy.MotherShip.Charge");
	
	// Enemy MotherShip Action
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_SpawnDrone, "Ability.Enemy.MotherShip.SpawnDrone");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_BombingRun, "Ability.Enemy_MotherShip_BombingRun");
	
	// Enemy Action
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_BasicMelee, "Ability.Enemy.BasicMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_DoubleMelee, "Ability.Enemy.DoubleMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_TripleMelee, "Ability.Enemy.TripleMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_StrongMelee, "Ability.Enemy.StrongMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_RangerAttack, "Ability.Enemy.RangerAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_FlyingBurstAttack, "Ability.Enemy.FlyingBurstAttack");
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_TitanWalker_MachineGun, "Ability.Enemy.TitanWalker.MachineGun");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_TitanWalker_Flame, "Ability.Enemy.TitanWalker.Flame");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_TitanWalker_Bombard, "Ability.Enemy.TitanWalker.Bombard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_TitanWalker_Laser, "Ability.Enemy.TitanWalker.Laser");


	// Enemy Action - Common
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Attack, "Ability.Enemy.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_HitReaction, "Ability.Enemy.HitReaction");

	// Enemy Event
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Hit, "Event.Enemy.Hit");
}

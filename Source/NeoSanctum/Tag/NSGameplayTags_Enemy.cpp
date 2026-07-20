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
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_MotherShip_Stealth, "State.Enemy.MotherShip.Stealth");
	
	// Enemy MotherShip Action
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_SpawnDrone, "Ability.Enemy.MotherShip.SpawnDrone");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_BombingRun, "Ability.Enemy_MotherShip_BombingRun");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_HomingMissile, "Ability.Enemy.MotherShip.HomingMissile");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_CloakDeployRun, "Ability.Enemy.MotherShip.CloakDeployRun");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_MachineGun, "Ability.Enemy.MotherShip.MachineGun");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MotherShip_ChargeShield, "Ability.Enemy.MotherShip.ChargeShield");
	
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
	
	// Enemy Cosmetic
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_MachineGun_Fire, "Cosmetic.Enemy.TitanWalker.MachineGun.Fire");

	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Flame_Start, "Cosmetic.Enemy.TitanWalker.Flame.Start");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Flame_Update, "Cosmetic.Enemy.TitanWalker.Flame.Update");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Flame_Stop, "Cosmetic.Enemy.TitanWalker.Flame.Stop");

	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Bombard_Prepare, "Cosmetic.Enemy.TitanWalker.Bombard.Prepare");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Bombard_Launch, "Cosmetic.Enemy.TitanWalker.Bombard.Launch");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Bombard_Warning, "Cosmetic.Enemy.TitanWalker.Bombard.Warning");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Bombard_Impact, "Cosmetic.Enemy.TitanWalker.Bombard.Impact");

	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Laser_ChargeStart, "Cosmetic.Enemy.TitanWalker.Laser.ChargeStart");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Laser_ChargeUpdate, "Cosmetic.Enemy.TitanWalker.Laser.ChargeUpdate");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Laser_BeamStart, "Cosmetic.Enemy.TitanWalker.Laser.BeamStart");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Laser_BeamUpdate, "Cosmetic.Enemy.TitanWalker.Laser.BeamUpdate");
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Laser_Stop, "Cosmetic.Enemy.TitanWalker.Laser.Stop");
	
	UE_DEFINE_GAMEPLAY_TAG(Cosmetic_Enemy_TitanWalker_Death_Explosion, "Cosmetic.Enemy.TitanWalker.Death.Explosion");
}

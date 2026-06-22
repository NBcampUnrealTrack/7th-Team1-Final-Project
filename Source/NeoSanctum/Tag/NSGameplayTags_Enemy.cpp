#include "NSGameplayTags_Enemy.h"

namespace NSGameplayTags
{
	// Enemy State
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_Combat, "State.Enemy.Combat");

	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_HitReacting, "State.Enemy.HitReacting");

	// Enemy Action
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_BasicMelee, "Ability.Enemy.BasicMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_DoubleMelee, "Ability.Enemy.DoubleMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_TripleMelee, "Ability.Enemy.TripleMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_StrongMelee, "Ability.Enemy.StrongMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_RangerAttack, "Ability.Enemy.RangerAttack");

	// Enemy Action - Common
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Attack, "Ability.Enemy.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_HitReaction, "Ability.Enemy.HitReaction");

	// Enemy Event
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Hit, "Event.Enemy.Hit");
}

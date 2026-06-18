#include "NSGameplayTags_Enemy.h"

namespace NSGameplayTags
{
	// Enemy State
	UE_DEFINE_GAMEPLAY_TAG(State_Enemy_Combat, "State.Enemy.Combat");

	// Enemy Action
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_BasicMelee, "Ability.Enemy.BasicMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_DoubleMelee, "Ability.Enemy.DoubleMelee");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_RangerAttack, "Ability.Enemy.RangerAttack");

	// Enemy Event
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_Hit, "Event.Enemy.Hit");
}

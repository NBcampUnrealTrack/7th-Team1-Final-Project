#include "NSGameplayTags_Companion.h"

namespace NSGameplayTags
{
	// 어빌리티 태그
	UE_DEFINE_GAMEPLAY_TAG(Ability_Companion_Fire, "Ability.Companion.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Companion_Active, "Ability.Companion.Active");
	
	// 스테이트 태그
	UE_DEFINE_GAMEPLAY_TAG(State_Companion_Disable, "State.Companion.Disable");
	
	// 데이터 태그
	UE_DEFINE_GAMEPLAY_TAG(Data_Companion_Damage, "Data.Companion.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_Companion_CoolDown, "Data.Companion.CoolDown");
	
	// 쿨다운 태그
	UE_DEFINE_GAMEPLAY_TAG(CoolDown_Companion_Fire, "CoolDown.Companion.Fire");
	
	// 업그레이드 주체 드론 태그
	UE_DEFINE_GAMEPLAY_TAG(Companion_Basic, "Companion.Basic");
	UE_DEFINE_GAMEPLAY_TAG(Companion_Attack, "Companion.Attack");
	
	// 업그레이드 하위 노드 태그
	UE_DEFINE_GAMEPLAY_TAG(Upgrade_Companion_Basic, "Upgrade.Companion.Basic");
	
	UE_DEFINE_GAMEPLAY_TAG(Upgrade_Companion_AttackDamage, "Upgrade.Companion.AttackDamage");
	UE_DEFINE_GAMEPLAY_TAG(Upgrade_Companion_FireRate, "Upgrade.Companion.FireRate");
	UE_DEFINE_GAMEPLAY_TAG(Upgrade_Companion_FireRange, "Upgrade.Companion.FireRange");
	
	// 공통 사용 업그레이드 태그
	UE_DEFINE_GAMEPLAY_TAG(Data_Upgrade, "Data.Upgrade");
}

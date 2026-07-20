#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// 어빌리티 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Companion_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Companion_Active);
	
	// 스테이트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Companion_Disable);
	
	// 데이터 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Companion_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Companion_CoolDown);
	
	// 쿨다운 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CoolDown_Companion_Fire);
	
	// 업그레이드 주체 드론 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Companion_Basic);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Companion_Attack);
	
	// 업그레이드 하위 노드 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Upgrade_Companion_Basic);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Upgrade_Companion_AttackDamage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Upgrade_Companion_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Upgrade_Companion_FireRange);
	
	// 공통 사용 업그레이드 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Upgrade);
	
	// 펫 강화 노드 UI 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_PetUpgrade_State_Locked);      // 잠김(해금 조건 미충족)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_PetUpgrade_State_Upgradable);  // 강화 가능(스탯 노드)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_PetUpgrade_State_Maxed);       // 만렙(스탯 노드, 버튼 off)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_PetUpgrade_State_Selectable);  // 선택 가능(드론 노드)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_PetUpgrade_State_Selected);    // 현재 선택됨(드론 노드)
}

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// GMS 쿨다운 상태 변경 채널
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_SkillCooldown_Changed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_Crosshair_AttackFeedback);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_HitTakenFeedback);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_HitTakenFeedback_State);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_HitTakenFeedback_Vitals);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_DamageNumber);
	
	// 펫 강화 상태 조회 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Query);

	// 펫 강화 화면 전체 상태 응답
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Snapshot);

	// 특정 노드 강화 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Upgrade_Request);

	// 특정 노드 강화 결과
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Upgrade_Result);
	
	//팀원 상태 목록 조회 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_TeammateStatus_Query);
	//팀원 상태 전체 Snapshot응답
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_TeammateStatus_Snapshot);
	//특정 팀원의 체력, 쉴드 상태 변경
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_TeammateStatus_Changed);
	//캐릭터 스텟 표시 Snapshot
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_CharacterStats_Snapshot);
	
	// 일반 몬스터 상태 UI 표시 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_NormalMonster_Reveal);
}

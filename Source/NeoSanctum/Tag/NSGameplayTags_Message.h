// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// GMS 쿨다운 상태 변경 채널
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_SkillCooldown_Changed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_Crosshair_AttackFeedback);
	
	// 펫 강화 상태 조회 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Query);

	// 펫 강화 화면 전체 상태 응답
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Snapshot);

	// 특정 노드 강화 요청
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Upgrade_Request);

	// 특정 노드 강화 결과
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_UI_PetUpgrade_Upgrade_Result);
}

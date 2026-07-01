// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Message.h"

namespace NSGameplayTags
{
	// GMS 쿨다운 상태 변경 채널
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_SkillCooldown_Changed, "Message.UI.SkillCooldown.Changed");
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_Crosshair_AttackFeedback, "Message.UI.Crosshair.AttackFeedback");
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_HitTakenFeedback, "Message.UI.HitTakenFeedback");
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_HitTakenFeedback_State, "Message.UI.HitTakenFeedback.State");
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_HitTakenFeedback_Vitals, "Message.UI.HitTakenFeedback.Vitals");
	
	// 펫 강화 전체 상태 조회 요청 채널
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_PetUpgrade_Query,"Message.UI.PetUpgrade.Query");
	// 펫 강화 전체 Snapshot 응답 채널
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_PetUpgrade_Snapshot,"Message.UI.PetUpgrade.Snapshot");
	// 특정 노드 강화 요청 채널
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_PetUpgrade_Upgrade_Request,"Message.UI.PetUpgrade.Upgrade.Request");
	// 강화 성공 또는 실패 결과 채널	
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_PetUpgrade_Upgrade_Result,"Message.UI.PetUpgrade.Upgrade.Result");
	//팀원 상태 목록 조회 요청
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_TeammateStatus_Query,"Message.UI.TeammateStatus.Query");
	//팀원 상태 전체 Snapshot응답 채널
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_TeammateStatus_Snapshot,"Message.UI.TeammateStatus.Snapshot");
	//특정 팀원의 체력/쉴드상태 변경 채널
	UE_DEFINE_GAMEPLAY_TAG(Message_UI_TeammateStatus_Changed,"Message.UI.TeammateStatus.Changed");
}

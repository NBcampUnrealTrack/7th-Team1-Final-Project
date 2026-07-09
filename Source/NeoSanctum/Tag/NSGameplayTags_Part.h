// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	/**
	 * Arm, Leg GE에 CurrentValue 주입 시 사용
	 * SetByCaller로 태그로 넘겨준 값을 쓰게끔
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Value);

	// 기존 게임플레이 파츠 슬롯 - 거래/장착/스탯이 있는 대상
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Slot_Arm);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Slot_Body);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Slot_Leg);

	// 시각 전용 슬롯 - 거래/장착 대상 아님, CharacterData 기본 외형(DefaultVisualParts)에만 사용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Hair);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Boots);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Jetpack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_BootsArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Head);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Hands);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_HandsArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Helm);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_UpperLegArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_LowerLegArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_ShoulderPads);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Kneepads);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Waist);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_UpperArmArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_LowerArmArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Visual_Torso);
}

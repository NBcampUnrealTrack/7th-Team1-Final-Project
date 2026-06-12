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
}

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// 추첨 시 사용할 풀 식별자
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Pool_Normal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Pool_HighGrade);

	// 스택 GE의 Magnitude 전달용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_SetByCaller_Stack);
}

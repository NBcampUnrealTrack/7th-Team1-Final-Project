// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;

namespace NSDamageRules
{
	// 데미지 GE를 적용하기 직전에 호출하는 공통 판정 함수.
	// 같은 팀, 자기 자신, ASC가 없거나 이미 죽은 대상, Health가 없는 대상은 피해 대상에서 제외
	bool CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor);

	// Target ASC만 알고 있을 때 생존/피해 가능 상태만 검사
	// Health Attribute가 있고 Health가 0보다 큰 대상만 피해를 입힐 수 있는 대상으로 판단함
	bool IsAliveDamageableTarget(const UAbilitySystemComponent* TargetASC);
}

// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;
struct FHitResult;

namespace NSDamageRules
{
	// 데미지 GE를 적용하기 직전에 호출하는 공통 판정 함수.
	// 같은 팀, 자기 자신, ASC가 없거나 이미 죽은 대상, Health가 없는 대상은 피해 대상에서 제외
	bool CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor);

	// Target ASC만 알고 있을 때 생존/피해 가능 상태만 검사
	// Health Attribute가 있고 Health가 0보다 큰 대상만 피해를 입힐 수 있는 대상으로 판단함
	bool IsAliveDamageableTarget(const UAbilitySystemComponent* TargetASC);
	
	// 직접 공격 HitResult가 Enemy의 현재 피격 정책에 맞는지 확인하는 함수
	bool IsValidDirectDamageHit(const FHitResult& HitResult);

	// 직접 공격 HitResult에서 부위별 데미지 배율을 반환하는 함수
	float ResolveDirectHitDamageMultiplier(const FHitResult& HitResult);
}

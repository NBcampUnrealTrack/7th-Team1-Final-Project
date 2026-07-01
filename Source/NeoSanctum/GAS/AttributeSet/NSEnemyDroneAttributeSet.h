// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSEnemyDroneAttributeSet.generated.h"

class ANSEnemyDroneAI;

UCLASS()
class NEOSANCTUM_API UNSEnemyDroneAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()
public:
	// instant GE로 Damage가 들어온 직후 호출 — 데미지 감지 보고와 드론 사망 판정을 수행
	virtual void PostGameplayEffectExecute(
		const FGameplayEffectModCallbackData& Data   // 적용된 GE의 공격자/피격자/수치 정보
	) override;

private:
	// AI 데미지 감지(AISense_Damage)로 "누가 나를 때렸는지"를 보고하는 함수 (어그로 전환의 입력원)
	void ReportDamageSenseEvent(
		const FGameplayEffectModCallbackData& Data   // 데미지 GE 콜백 데이터 (공격자/피격자 추출)
	) const;

	// 드론 공격자를 실제 어그로 대상(컴패니언이면 그 오너 플레이어)으로 변환하는 함수
	AActor* ResolvePerceivedInstigator(
		AActor* InstigatorActor   // GE의 원 instigator
	) const;

	// 체력이 0 이하가 된 드론을 사망 처리하는 함수
	void HandleDeathAfterEffect(
		ANSEnemyDroneAI* EnemyDrone   // 사망시킬 드론 폰
	) const;
};

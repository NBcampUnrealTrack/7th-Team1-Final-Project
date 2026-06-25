// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSSkillCooldownTypes.generated.h"

// 스킬 슬롯 UI에 전달할 쿨다운 상태 데이터
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FSkillCooldownUIData
{
	GENERATED_BODY()

	// 조회 대상 스킬 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	FGameplayTag SkillSlotTag;

	// 현재 재충전 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	bool bIsRecharging = false;

	// 남은 쿨다운 시간
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	float RemainingTime = 0.0f;

	// 전체 쿨다운 시간
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	float TotalTime = 0.0f;

	// UI 표시용 진행도
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	float NormalizedProgress = 1.0f;

	// 현재 스킬 충전 수
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	int32 CurrentCount = 0;

	// 최대 스킬 충전 수
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	int32 MaxCount = 0;
};

/**
 *  스킬 쿨타임 UI표시 요청 메시지
 *
 *  GAS에서 쿨타임을 관리하고
 *  UI에서 메세지에 담긴 시간을 이용해 시각효과 재생
 */
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSSkillCooldownMessage
{
	GENERATED_BODY()

	// 주현 : 슬롯 기준으로 쿨다운 UI를 갱신하기 위한 신규 데이터
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag SkillSlotTag;

	// 주현 : UI 담당자가 우선 사용해야 하는 쿨다운 상태 데이터
	UPROPERTY(BlueprintReadWrite)
	FSkillCooldownUIData CooldownData;
};

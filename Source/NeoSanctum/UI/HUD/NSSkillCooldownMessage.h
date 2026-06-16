// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "NSSkillCooldownMessage.generated.h"


//GMS에서 사용할 쿨타임 시작 채널
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_UI_SkillCooldown_Start);


/**
 *  스킬 쿨타임 UI표시 요청 메시지
 *  
 *  GAS에서 쿨타임을 관리하고
 *  UI에서 메세지에 담긴 시간을 이용해 시각효과 재생
 */
USTRUCT(BlueprintType)
struct FNSSkillCooldownMessage
{
	GENERATED_BODY()
	
	//어떤 스킬인지 구분
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag SkillTag;
	
	//실제 적용된 쿨타임 태그
	//태그를 쿼리로 검사해 어떤 슬롯이 반응할지 판단.
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag CooldownTag;

	//UI가 표시하는 쿨타임 시간
	UPROPERTY(BlueprintReadWrite)
	float CooldownDuration = 0.0f;

	//충전형 스킬에서 현재 사용 가능한 횟수
	UPROPERTY(BlueprintReadWrite)
	int32 CurrentCharge = 0;

	//충전형 스킬에서 최대 충전 횟수
	UPROPERTY(BlueprintReadWrite)
	int32 MaxCharge = 0;
};

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
	
	/*
	 * 대쉬 
	 */
	
	// 어떤 스킬인지 구분
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag SkillTag;

	// 다음 충전까지 필요한 시간
	UPROPERTY(BlueprintReadWrite)
	float CooldownDuration = 0.0f;

	// 현재 사용 가능한 횟수
	UPROPERTY(BlueprintReadWrite)
	int32 CurrentCharge = 0;

	// 최대 충전 횟수
	UPROPERTY(BlueprintReadWrite)
	int32 MaxCharge = 0;
};

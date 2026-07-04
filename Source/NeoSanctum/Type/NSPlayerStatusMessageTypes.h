// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSPlayerStatusMessageTypes.generated.h"

/**
 * 팀원 상태가 UI현재 팀원 목록을 요청할때 사용하는 메시지
 */

USTRUCT(BlueprintType)
struct FNSPlayerStatusQueryMessage
{
	GENERATED_BODY()
	
	//요청과 Snapshot 응답을 연결하는 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;
};

/**
 * 팀원 한명의 상태를 UI에 표시
 */

USTRUCT(BlueprintType)
struct FNSPlayerStatusViewData
{
	GENERATED_BODY()
	
	//현재 세션에서 플레이어를 구분하는 식별자
	UPROPERTY(BlueprintReadOnly)
	int32 PlayerId = INDEX_NONE;

	//UI에 표시할 플레이어 이름
	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	//현재 체력
	UPROPERTY(BlueprintReadOnly)
	float CurrentHealth = 0.0f;

	//최대 체력
	UPROPERTY(BlueprintReadOnly)
	float MaxHealth = 0.0f;

	//현재 쉴드
	UPROPERTY(BlueprintReadOnly)
	float CurrentShield = 0.0f;

	//최대 쉴드
	UPROPERTY(BlueprintReadOnly)
	float MaxShield = 0.0f;
	
	//UI는 AttributeSet을 직접 참조하지않고 GMS데이터만 사용

	//플레이어 사망 여부
	UPROPERTY(BlueprintReadOnly)
	bool bIsDead = false;
};

/**
 * 팀원 목록 전체를 UI에 전달하는 Snapshot 메시지입니다.
 */
USTRUCT(BlueprintType)
struct FNSPlayerStatusSnapshotMessage
{
	GENERATED_BODY()

	//Query 메시지와 연결되는 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	//로컬 플레이어를 제외한 팀원 목록
	UPROPERTY(BlueprintReadOnly)
	TArray<FNSPlayerStatusViewData> Players;
};

/**
 * 특정 팀원의 상태 변경 또는 퇴장을 UI에 전달하는 메시지입니다.
 */
USTRUCT(BlueprintType)
struct FNSPlayerStatusChangedMessage
{
	GENERATED_BODY()

	//변경된 팀원의 최신 상태
	UPROPERTY(BlueprintReadOnly)
	FNSPlayerStatusViewData StatusData;

	//true이면 해당 플레이어가 목록에서 제거되었음을 의미
	UPROPERTY(BlueprintReadOnly)
	bool bRemoved = false;
};

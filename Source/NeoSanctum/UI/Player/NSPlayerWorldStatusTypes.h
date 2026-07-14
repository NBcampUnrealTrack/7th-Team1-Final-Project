// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSPlayerWorldStatusTypes.generated.h"

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 구조체 개요: 플레이어 월드 상태 UI에 전달할 표시 데이터를 정의합니다.
 * Widget은 Attribute를 직접 계산하지 않고, 체력 비율과 표시 가능 여부만 화면에 반영합니다.
 */
USTRUCT(BlueprintType)
struct FNSPlayerWorldStatusData
{
	GENERATED_BODY()

	// 상태를 표시할 플레이어 식별자를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "PlayerWorldStatus")
	int32 PlayerId = INDEX_NONE;

	// 상태를 표시할 플레이어 이름을 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "PlayerWorldStatus")
	FText PlayerName;

	// 체력 진행도를 0~1로 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "PlayerWorldStatus")
	float HealthPercent = 1.0f;

	// 플레이어 사망 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "PlayerWorldStatus")
	bool bIsDead = false;

	// 월드 상태 UI 표시 가능 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "PlayerWorldStatus")
	bool bVisible = true;
};

// 플레이어 월드 상태 변경을 위젯에 알리는 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FNSPlayerWorldStatusChanged, const FNSPlayerWorldStatusData&);

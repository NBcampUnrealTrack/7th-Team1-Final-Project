// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSDropLaunchData.generated.h"

/**
 * 서버가 결정한 월드 드랍 발사 궤적 정보
 * 클라이언트마다 같은 시작점, 착지점, 포물선 연출을 재생하기 위해 사용
 */
USTRUCT()
struct FNSDropLaunchData
{
	GENERATED_BODY()
	
	// 네트워크 전송 시 정밀도를 일부 줄여 대역폭을 절약하는 시작 위치
	UPROPERTY()
	FVector_NetQuantize10 StartLocation = FVector::ZeroVector;
	
	UPROPERTY()
	FVector_NetQuantize10 TargetLocation = FVector::ZeroVector;
	
	UPROPERTY()
	float StartServerTime = 0.0f;
	
	UPROPERTY()
	float FlightDuration = 0.0f;
	
	UPROPERTY()
	float ArcHeight = 0.0f;
	
	bool IsValid() const
	{
		return FlightDuration > KINDA_SMALL_NUMBER && ArcHeight > 0.0f;
	}
};

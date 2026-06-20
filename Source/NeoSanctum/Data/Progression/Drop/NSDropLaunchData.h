// Copyright 2026 One Team. All rights reserved.

#pragma once

/**
 * 서버가 결정한 월드 드랍 발사 궤적 정보
 * 클라이언트마다 같은 시작점, 착지점, 포물선 연출을 재생하기 위해 사용
 */
USTRUCT()
struct FNSDropLaunchData
{
	GENERATED_BODY()
	
	// FVector_NetQuantize10 무슨 자료형인지 질문
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

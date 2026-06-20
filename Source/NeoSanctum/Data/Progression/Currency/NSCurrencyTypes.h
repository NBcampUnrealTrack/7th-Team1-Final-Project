// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Progression/Drop/NSDropLaunchData.h"
#include "NSCurrencyTypes.generated.h"

class ANSPlayerState;

UENUM(BlueprintType)
enum class ENSCurrencyGrade : uint8
{
	None UMETA(DisplayName="없음"),	
	Grade1 UMETA(DisplayName="1등급"),
	Grade2 UMETA(DisplayName="2등급"),
	Grade3 UMETA(DisplayName="3등급"),
};

/**
 * 서버 드랍 레지스트리 (서버 전용, 복제안함)
 */
USTRUCT()
struct FNSCurrencyDropEntry
{
	GENERATED_BODY()
	
	// 재화 종류(Currency.Temp / Common / Skill)
	UPROPERTY()
	FGameplayTag CurrencyType;
	
	// 등급 (영구재화의 경우 None)
	UPROPERTY()
	ENSCurrencyGrade Grade = ENSCurrencyGrade::None;
	
	// 획득 양
	UPROPERTY()
	int64 Amount = 0;
	
	// 드랍 위치 (오버랩 거리 검증용)
	UPROPERTY()
	FVector Location = FVector::ZeroVector;
	
	// 재화 픽업 비주얼의 포물선 발사 정보
	UPROPERTY()
	FNSDropLaunchData LaunchData;
	
	// 만료 시간
	UPROPERTY()
	float ExpireTime = 0.f;
	
	// 이미 획득한 플레이어
	UPROPERTY()
	TSet<TWeakObjectPtr<ANSPlayerState>> CollectedPlayer;
};

/**
 * 네트워크로 클라까지 전송되는 재화 생성 정보
 * CollectedPlayer처럼 누가 획득했는지에 대한 정보가 없음
 */
USTRUCT()
struct FNSCurrencySpawnEvent
{
	GENERATED_BODY()
	
	// 드롭 식별 번호
	UPROPERTY()
	int32 DropId = INDEX_NONE;
	
	// 재화 종류 (비주얼 데이터 조회 및 픽업 라우팅 용)
	UPROPERTY()
	FGameplayTag CurrencyType;
	
	// 통화 등급
	UPROPERTY()
	ENSCurrencyGrade Grade = ENSCurrencyGrade::None;
	
	// 획득 양
	UPROPERTY()
	int64 Amount = 0;
	
	// 드랍 위치, FVector_NetQuantize10 : 전송용 벡터 압축
	UPROPERTY()
	FVector_NetQuantize10 Location = FVector::ZeroVector;
	
	// 클라이언트에서 동일한 포물선 궤적을 재생하기 위한 서버 결정 발사 정보
	FNSDropLaunchData LaunchData;

	// 만료까지 남은 시간
	UPROPERTY()
	float Duration = 0.f;
};
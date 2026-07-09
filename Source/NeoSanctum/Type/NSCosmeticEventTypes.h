// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSCosmeticEventTypes.generated.h"

UENUM(BlueprintType)
enum class ENSCosmeticEventPhase : uint8
{
	// 한 번만 재생되는 단발 코스메틱 상태
	OneShot,

	// 지속형 코스메틱이 시작되는 상태
	Start,

	// 지속형 코스메틱의 위치, 방향, 크기 등이 갱신되는 상태
	Update,

	// 지속형 코스메틱이 종료되는 상태
	Stop
};

USTRUCT(BlueprintType)
struct FNSCosmeticEventPointNetData
{
	GENERATED_BODY()

	// 이벤트 시작 위치를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantize Location = FVector::ZeroVector;

	// 이벤트 끝 위치를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantize EndLocation = FVector::ZeroVector;

	// 이벤트 진행 방향을 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;
};

USTRUCT(BlueprintType)
struct FNSCosmeticEventNetData
{
	GENERATED_BODY()

	// 실행할 코스메틱 이벤트 종류를 식별하는 변수
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag EventTag;

	// 지속형 코스메틱의 Start, Update, Stop을 묶는 변수
	UPROPERTY(BlueprintReadWrite)
	int32 InstanceId = INDEX_NONE;

	// 이벤트의 현재 단계를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	ENSCosmeticEventPhase Phase = ENSCosmeticEventPhase::OneShot;

	// 이벤트 대표 시작 위치를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantize Location = FVector::ZeroVector;

	// 이벤트 대표 끝 위치를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantize EndLocation = FVector::ZeroVector;

	// 이벤트 대표 진행 방향을 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;

	// 판정 반경 또는 표시 반경을 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	float Radius = 0.0f;

	// 표시 길이 또는 공격 사거리를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	float Range = 0.0f;

	// 경고 시간 또는 지속 시간을 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	float Duration = 0.0f;

	// 여러 소켓, 빔, 탄착점 정보를 저장하는 변수
	UPROPERTY(BlueprintReadWrite)
	TArray<FNSCosmeticEventPointNetData> Points;
};

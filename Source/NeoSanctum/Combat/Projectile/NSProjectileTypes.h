// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSProjectileTypes.generated.h"

class UGameplayEffect;

/**
 * 원거리 공격 GA가 투사체 Manager에 전달하는 발사 요청 정보
 */
USTRUCT()
struct FNSProjectileFireRequest
{
	GENERATED_BODY()
	
	// 투사체 생성 위치
	FVector StartLocation = FVector::ZeroVector;

	// 투사체 이동 방향
	FVector Direction = FVector::ForwardVector;

	// 투사체 초당 이동 거리
	float Speed = 1000.0f;

	// 투사체 최대 수명 시간
	float MaxLifeTime = 5.0f;

	// 투사체 충돌 반지름
	float Radius = 10.0f;

	// 투사체 Trace Channel
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 투사체를 발사한 Enemy: Enemy가 죽어도 투사체가 Actor의 수명을 강제로 유지하지 않음
	TWeakObjectPtr<AActor> SourceActor = nullptr;
	
	// 충돌 대상에게 적용할 GameplayEffect 클래스
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};

/**
 * 서버에서 관리되는 투사체 런타임 상태 정보
 * 
 * Actor를 생성하지 않고 Manager의 TArray 안에 구조체 저장
 * 매 Tick마다 CurrentLocation과 LifeTime만 갱신
 */
USTRUCT()
struct FNSServerProjectileData
{
	GENERATED_BODY()
	
	// 현재 투사체 위치
	FVector CurrentLocation = FVector::ZeroVector;

	// 투사체 이동 방향
	FVector Direction = FVector::ForwardVector;

	// 투사체 초당 이동 거리
	float Speed = 0.0f;

	// 투사체 경과 시간
	float LifeTime = 0.0f;

	// 투사체 최대 수명 시간
	float MaxLifeTime = 0.0f;

	// 투사체 충돌 반지름
	float Radius = 0.0f;

	// 투사체 Trace Channel
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 투사체를 발사한 Enemy
	TWeakObjectPtr<AActor> SourceActor = nullptr;
	
	// 충돌 대상에게 적용할 GameplayEffect 클래스
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};

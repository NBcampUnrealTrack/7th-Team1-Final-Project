// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSEnemyAgent.generated.h"

class UNSEnemyData;
class UAbilitySystemComponent;
class USkeletalMeshComponent;
struct FNSEnemyAttackRow;

/**
 * Character, Boss Pawn, Drone Pawn을 공통 Enemy로 다루기 위한 Interface입니다.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UNSEnemyAgent : public UInterface
{
	GENERATED_BODY()
};

class NEOSANCTUM_API INSEnemyAgent
{
	GENERATED_BODY()

public:
	// 데이터 에셋을 반환하는 함수
	virtual UNSEnemyData* GetEnemyData() const = 0;

	// 메시를 반환하는 함수
	virtual USkeletalMeshComponent* GetEnemyMesh() const = 0;

	// 현재 실행 중인 공격 Row를 반환하는 함수
	virtual const FNSEnemyAttackRow* GetCurrentAttackRow() const = 0;

	// 현재 실행할 공격 Row를 저장하는 함수
	virtual void SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow) = 0;

	// 현재 공격 Row를 초기화하는 함수
	virtual void ClearCurrentAttackRow() = 0;

	// 이 Enemy가 피격 경직 상태인지 반환하는 함수
	virtual bool IsHitReacting() const = 0;

	// 공격, 조준, Trace 기준으로 사용할 위치를 반환하는 함수
	virtual FVector GetAimLocation() const = 0;
};

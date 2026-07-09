// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NSEnemyCombatComponent.generated.h"

/**
 * Enemy의 현재 공격 상태를 Character/Pawn 공통으로 관리하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyCombatComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 실행할 공격 Row를 저장하는 함수
	void SetAttackRow(const FNSEnemyAttackRow& InAttackRow);

	// 현재 실행 중인 공격 Row를 반환하는 함수
	const FNSEnemyAttackRow* GetAttackRow() const;

	// 현재 공격 Row를 초기화하는 함수
	void ClearAttackRow();

	// 현재 공격 Row가 유효한지 반환하는 함수
	bool HasCurrentAttackRow() const { return bHasCurrentAttackRow; }

	// 현재 공격의 AttackId를 반환하는 함수
	FName GetCurrentAttackId() const;

private:
	// 현재 실행 중인 공격 Row
	UPROPERTY(Transient)
	FNSEnemyAttackRow CurrentAttackRow;

	// 현재 공격 Row가 유효한지 여부
	UPROPERTY(Transient)
	bool bHasCurrentAttackRow = false;
	
	// 클라이언트 ABP가 현재 공격 Row를 EnemyData에서 다시 찾기 위해 사용하는 공격 ID를 복제하는 변수
	UPROPERTY(Replicated)
	FName ReplicatedAttackId = NAME_None;
};
